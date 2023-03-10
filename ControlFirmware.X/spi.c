#include <xc.h>
#include <stdint.h>
#include "spi.h"
#include "main.h"
#include "peripherals/ports.h"

#define _XTAL_FREQ 64000000

#define SPI_DEVICE_COUNT 4
#define SPI_QUEUE_COUNT  8

spi_device_t *spi_devices[SPI_DEVICE_COUNT];
spi_command_t *spi_queue[SPI_QUEUE_COUNT];
spi_command_t *spi_command = NULL;
uint8_t spi_last_device = 0xff;

const uint8_t spi_baud[SPI_BAUD_COUNT] = { 255, 159, 79, 39, 19, 9 };

void spi_initialise(void) {  
    for (uint8_t i = 0; i < SPI_DEVICE_COUNT; i++) {
        spi_devices[i] = NULL;
    }
    
    for (uint8_t i = 0; i < SPI_QUEUE_COUNT; i++) {
        spi_queue[i] = NULL;
    }
    
    spi_command = NULL;
    
    /* Set SPI clock ready for baud rates. */
    SPI2CLK = 0b000000; /* 64MhHz clock. */
    SPI2BAUD = spi_baud[0]; /* Default to lowest speed. */
    /* Set to SPI master. */
    SPI2CON0bits.MST = 1;
    /* Set to bit mode, and set width to be 8. */
    SPI2CON0bits.BMODE = 1;
    SPI2TWIDTHbits.TWIDTH = 0;    
    /* Enable zero count interrupt. */
    SPI2INTEbits.TCZIE = 1;
    
    PIE5bits.SPI2IE = 1;
    PIE6bits.DMA2DCNTIE = 1;
}

void spi_service(void) {   
    if (spi_command != NULL && (
            ((spi_command->operation == SPI_OPERATION_WRITE || spi_command->operation == SPI_OPERATION_WRITE_THEN_READ) && (SPI2INTFbits.TCZIF == 1 && SPI2CON2bits.BUSY == 0)) || 
            (spi_command->operation == SPI_OPERATION_READ && PIR6bits.DMA2DCNTIF == 1))) {
        
        spi_command->in_progress = 0;

        SPI2CON0bits.EN = 0;
        DMASELECT = 1;
        DMAnCON0bits.EN = 0; 
        
        if (spi_command->operation == SPI_OPERATION_WRITE_THEN_READ) {
            spi_command->operation = SPI_OPERATION_READ;       
            __delay_us(10);
        } else {
            bool cs_high = true;
            uint8_t previous_device = spi_command->device;
                                
            if (spi_command->callback != NULL) {
                spi_command = spi_command->callback(spi_command);   
                
                if (spi_command != NULL && spi_command->device == previous_device) {
                    if (!spi_devices[previous_device]->cs_bounce) {
                        cs_high = false;
                    }
                }
            } else {
                spi_command = NULL;
            }   
            
            if (cs_high) {
                *(kpin_to_rxypps(spi_devices[previous_device]->clk_pin)) = 0x00;
                *(kpin_to_rxypps(spi_devices[previous_device]->mosi_pin)) = 0x00;
            
                kpin_write(spi_devices[previous_device]->cs_pin, true);                
            }
        }
    }
    
    if (spi_command == NULL) {
        spi_command = spi_queue[0];
        
        for (uint8_t i = 1; i < SPI_QUEUE_COUNT; i++) {
            spi_queue[i-1] = spi_queue[i];
        }
        
        spi_queue[SPI_QUEUE_COUNT - 1] = NULL;
    }
        
    if (spi_command != NULL && !spi_command->in_progress) {                   
        kpin_write(spi_devices[spi_command->device]->cs_pin, false);

        for (uint16_t i = 0; i < spi_command->cs_delay; i++) {
            __delay_us(1);
        }
        
        *(kpin_to_rxypps(spi_devices[spi_command->device]->clk_pin)) = 0x34;
        *(kpin_to_rxypps(spi_devices[spi_command->device]->mosi_pin)) = 0x35;
        SPI2SDIPPS = kpin_to_ppspin(spi_devices[spi_command->device]->miso_pin);
                                
        SPI2BAUD = spi_baud[spi_devices[spi_command->device]->baud];
        
        SPI2CON0bits.LSBF = spi_devices[spi_command->device]->lsb_first;
        SPI2CON1bits.CKE = 1;
        
        DMASELECT = 1;        
        DMAnCON1bits.SMR = 0b00;
                
        switch(spi_command->operation) {
            case SPI_OPERATION_WRITE:
            case SPI_OPERATION_WRITE_THEN_READ:
                SPI2CON2bits.RXR = 0;
                SPI2CON2bits.TXR = 1;
                
                DMAnCON1bits.SMODE = 0b01;
                DMAnCON1bits.DMODE = 0b00;

                SPI2TCNT = spi_command->write_size;
                DMAnSSZ = spi_command->write_size;
                DMAnDSZ = 1;
                
                DMAnSSA = spi_command->buffer;
                DMAnDSA = &SPI2TXB;

                DMAnCON1bits.SSTP = 1;
                DMAnCON1bits.DSTP = 0;
                
                DMAnSIRQ = 0x29;                
                break;
                
            case SPI_OPERATION_READ:
                SPI2CON2bits.RXR = 1;
                SPI2CON2bits.TXR = 0;
                
                DMAnCON1bits.SMODE = 0b00;
                DMAnCON1bits.DMODE = 0b01;
                
                SPI2TCNT = spi_command->read_size;
                DMAnSSZ = 1;
                DMAnDSZ = spi_command->read_size;
                
                DMAnSSA = &SPI2RXB;
                DMAnDSA = spi_command->buffer;

                DMAnCON1bits.SSTP = 0;
                DMAnCON1bits.DSTP = 1;
                
                DMAnSIRQ = 0x28;       
                
                break;
        } 
        
        PIR6bits.DMA2DCNTIF = 0;
        SPI2STATUSbits.CLRBF = 1;
        
        SPI2CON0bits.EN = 1;                
        DMAnCON0bits.SIRQEN = 1;
        DMAnCON0bits.EN = 1;        
        
        spi_command->in_progress = 1;
    }    
}

uint8_t spi_register(spi_device_t *d) {
    for (uint8_t i = 0; i < SPI_DEVICE_COUNT; i++) {
        if (spi_devices[i] == NULL) {
            spi_devices[i] = d;
            return i;
        }
    }
    
    return 0xff;
}

void spi_enqueue(spi_command_t *c) {
    for (uint8_t i = 0; i < SPI_QUEUE_COUNT; i++) {
        if (spi_queue[i] == NULL) {
            spi_queue[i] = c;
            return;
        }
    }  
}
