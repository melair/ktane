#include <xc.h>
#include <stdint.h>
#include "spi.h"
#include "main.h"
#include "peripherals/ports.h"

#define SPI_DEVICE_COUNT 4
#define SPI_QUEUE_COUNT  8

spi_device_t *spi_devices[SPI_DEVICE_COUNT];
spi_command_t *spi_queue[SPI_QUEUE_COUNT];
spi_command_t *spi_command = NULL;
uint8_t spi_last_device = 0xff;

const uint8_t spi_baud[SPI_BAUD_COUNT] = { 255, 159, 79, 39, 19, 9 };

void spi_initialise(void) {
    /* Unlock PPS. */
    pps_unlock();
            
    for (uint8_t i = 0; i < SPI_DEVICE_COUNT; i++) {
        spi_devices[i] = NULL;
    }
    
    for (uint8_t i = 0; i < SPI_QUEUE_COUNT; i++) {
        spi_queue[i] = NULL;
    }
    
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
}

void spi_service(void) {
    uint8_t previous_device = 0xff;
    
    if (spi_command != NULL && (
            (spi_command->operation == SPI_OPERATION_WRITE && SPI2INTFbits.TCZIF == 1) || 
            (spi_command->operation == SPI_OPERATION_READ && PIR6bits.DMA2DCNTIF == 1))) {
        spi_command->in_progress = 0;
        
        SPI2CON0bits.EN = 0;
        DMASELECT = 1;
        DMAnCON0bits.EN = 0; 
        
        previous_device = spi_command->device;
                        
        if (spi_command->callback != NULL) {
            spi_command = spi_command->callback(spi_command);   
        } else {
            spi_command = NULL;
        }             
    }
    
    if (spi_command == NULL) {
        spi_command = spi_queue[0];
        
        for (uint8_t i = 1; i < SPI_QUEUE_COUNT; i++) {
            spi_queue[i-1] = spi_queue[i];
        }
    }
        
    if (spi_command != NULL && !spi_command->in_progress) {
        if (spi_command->device != previous_device || (previous_device != 0xff && spi_devices[previous_device]->spi_cs_bounce_between)) {
            kpin_write(spi_devices[previous_device]->spi_cs, true);
        }
                
        kpin_write(spi_devices[spi_command->device]->spi_cs, false);
                
        *(spi_devices[spi_command->device]->spi_clk_pps_addr) = 0x34;
        *(spi_devices[spi_command->device]->spi_mosi_pps_addr) = 0x35;        
        SPI2SDIPPS = spi_devices[spi_command->device]->spi_miso_pps_port;
                        
        SPI2TCNT = spi_command->size;
        SPI2BAUD = spi_baud[spi_devices[spi_command->device]->baud];
        
        DMASELECT = 1;        
        DMAnCON1bits.SMR = 0b00;
        
        switch(spi_command->operation) {
            case SPI_OPERATION_WRITE:
                SPI2CON2bits.RXR = 0;
                SPI2CON2bits.TXR = 1;
                
                DMAnCON1bits.SMODE = 0b01;
                DMAnCON1bits.DMODE = 0b00;

                DMAnSSZ = spi_command->size;
                DMAnDSZ = 1;

                DMAnCON1bits.SSTP = 1;
                DMAnCON1bits.DSTP = 0;
                
                DMAnSIRQ = 0x29;                
                break;
                
            case SPI_OPERATION_READ:
                SPI2CON2bits.RXR = 1;
                SPI2CON2bits.TXR = 0;
                
                DMAnCON1bits.SMODE = 0b00;
                DMAnCON1bits.DMODE = 0b01;
                
                DMAnSSZ = 1;
                DMAnDSZ = spi_command->size;

                DMAnCON1bits.SSTP = 0;
                DMAnCON1bits.DSTP = 1;
                
                DMAnSIRQ = 0x28;       
                break;
        } 
        
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
            break;
        }
    }  
}
