#include <xc.h>
#include <stdint.h>
#include "spi.h"
#include "pins.h"
#include "../dma.h"

#define _XTAL_FREQ 64000000

spi_command_t *spi_queue_head = NULL;
spi_command_t *spi_queue_tail = NULL;
spi_command_t *spi_command = NULL;

const uint8_t spi_baud[SPI_BAUD_COUNT] = {255, 159, 79, 39, 19, 9, 4, 2};

void spi_initialise(void) {
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

        SPI2INTFbits.TCZIF = 0;
        PIR6bits.DMA2DCNTIF = 0;

        spi_command->in_progress = 0;

        SPI2CON0bits.EN = 0;
        DMASELECT = DMA_SPI;
        DMAnCON0bits.EN = 0;

        if (spi_command->operation == SPI_OPERATION_WRITE_THEN_READ) {
            spi_command->operation = SPI_OPERATION_READ;
        } else {
            bool cs_high = true;
            spi_device_t *previous_device = spi_command->device;

            if (spi_command->callback != NULL) {
                spi_command = spi_command->callback(spi_command);

                if (spi_command != NULL && spi_command->device == previous_device) {
                    if (!previous_device->cs_bounce) {
                        cs_high = false;
                    }
                }
            } else {
                spi_command = NULL;
            }

            *(kpin_to_rxypps(previous_device->mosi_pin)) = 0x00;
            kpin_write(previous_device->mosi_pin, false);
            if (cs_high) {
                *(kpin_to_rxypps(previous_device->clk_pin)) = 0x00;
                kpin_write(previous_device->cs_pin, true);
            }
        }
    }

    if (spi_command == NULL) {
        spi_command = spi_queue_head;

        if (spi_command != NULL) {
            spi_queue_head = spi_command->next_cmd;
        }

        if (spi_queue_head == NULL) {
            spi_queue_tail = NULL;
        }
    }

    if (spi_command != NULL && !spi_command->in_progress) {
        kpin_write(spi_command->device->cs_pin, false);

        for (uint16_t i = 0; i < spi_command->pre_delay; i++) {
            __delay_us(1);
        }

        *(kpin_to_rxypps(spi_command->device->clk_pin)) = 0x34;

        if (spi_command->operation == SPI_OPERATION_WRITE || spi_command->operation == SPI_OPERATION_WRITE_THEN_READ) {
            *(kpin_to_rxypps(spi_command->device->mosi_pin)) = 0x35;
        } else {
            kpin_write(spi_command->device->mosi_pin, true);
            SPI2SDIPPS = kpin_to_ppspin(spi_command->device->miso_pin);
        }

        SPI2BAUD = spi_baud[spi_command->device->baud];

        SPI2CON0bits.LSBF = spi_command->device->lsb_first ? 1 : 0;
        SPI2CON1bits.CKE = spi_command->device->cke ? 1 : 0;

        SPI2TWIDTHbits.TWIDTH = (spi_command->device->bits == 8 ? 0 : spi_command->device->bits);

        DMASELECT = DMA_SPI;
        DMAnCON1bits.SMR = 0b00;

        switch (spi_command->operation) {
            case SPI_OPERATION_WRITE:
            case SPI_OPERATION_WRITE_THEN_READ:
                SPI2CON2bits.RXR = 0;
                SPI2CON2bits.TXR = 1;

                DMAnCON1bits.SMODE = 0b01;
                DMAnCON1bits.DMODE = 0b00;

                SPI2TCNTH = (spi_command->write_size >> 8) & 0x07;
                SPI2TCNTL = spi_command->write_size & 0xff;
                DMAnSSZH = SPI2TCNTH;
                DMAnSSZL = SPI2TCNTL;
                DMAnDSZ = 1;

                DMAnSSA = (volatile uint24_t) spi_command->buffer;
                DMAnDSA = (volatile unsigned short) &SPI2TXB;

                DMAnCON1bits.SSTP = 1;
                DMAnCON1bits.DSTP = 0;

                DMAnSIRQ = 0x29;
                break;

            case SPI_OPERATION_READ:
                SPI2CON2bits.RXR = 1;
                SPI2CON2bits.TXR = 0;

                DMAnCON1bits.SMODE = 0b00;
                DMAnCON1bits.DMODE = 0b01;

                SPI2TCNTH = (spi_command->read_size >> 8) & 0x07;
                SPI2TCNTL = spi_command->read_size & 0xff;
                DMAnSSZ = 1;
                DMAnDSZH = SPI2TCNTH;
                DMAnDSZL = SPI2TCNTL;

                DMAnSSA = (volatile uint24_t) & SPI2RXB;
                DMAnDSA = (volatile unsigned short) spi_command->buffer;

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

void spi_enqueue(spi_command_t *c) {
    c->next_cmd = NULL;

    if (spi_queue_tail == NULL) {
        spi_queue_head = c;
    } else {
        spi_queue_tail->next_cmd = c;
    }

    spi_queue_tail = c;
}

spi_command_t *spi_unused_callback(spi_command_t *cmd) {
    return NULL;
}
