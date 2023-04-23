#include <xc.h>
#include <stdbool.h>
#include "i2c.h"
#include "../dma.h"

/*
 * This abstraction does not currently support RESTARTs of the I2C bus, when
 * RSEN is enabled we see issues with the I2C1CNT parameter being ignored, either
 * reading two extra bytes at the end or not reading enough bytes (when RSEN
 * is reset before S is issued).
 *
 * Most I2C slaves should be fine with this, as this is not a multimaster bus
 * there is no risk of another master preempting the follow on read.
 */

i2c_command_t *i2c_queue_head = NULL;
i2c_command_t *i2c_queue_tail = NULL;
i2c_command_t *i2c_command = NULL;

void i2c_initialise(void) {
    /* Configure pin SCK */
    ODCONCbits.ODCC3 = 1;
    TRISCbits.TRISC3 = 0;
    RC3I2Cbits.SLEW = 0b01;
    RC3I2Cbits.PU = 0b00;
    RC3I2Cbits.TH = 0b01;
    I2C1SCLPPS = 0b010011;
    RC3PPS = 0x37;
    WPUCbits.WPUC3 = 1;

    /* Configure pin SDA */
    ODCONCbits.ODCC4 = 1;
    TRISCbits.TRISC3 = 0;
    RC4I2Cbits.SLEW = 0b01;
    RC4I2Cbits.PU = 0b00;
    RC4I2Cbits.TH = 0b01;
    I2C1SDAPPS = 0b010100;
    RC4PPS = 0x38;
    WPUCbits.WPUC4 = 1;

    /* Host mode. */
    I2C1CON0bits.MODE = 0b100;

    /* 100kHz baud */
    I2C1CLKbits.CLK = 0b0000;
    I2C1BAUD = 31;

    /* Start I2C peripheral. */
    I2C1CON0bits.EN = 1;

    /* Enable interrupt to wake from sleep for DMA completion. */
    PIE2bits.DMA1DCNTIE = 1;
}

void i2c_service(void) {
    DMASELECT = DMA_I2C;

    if (i2c_command != NULL && (
            ((i2c_command->operation == I2C_OPERATION_WRITE) && (I2C1PIRbits.PCIF)) ||
            ((i2c_command->operation == I2C_OPERATION_WRITE_THEN_READ) && (I2C1PIRbits.PCIF /*|| I2C1CON0bits.MDR*/)) ||
            ((i2c_command->operation == I2C_OPERATION_READ && (I2C1PIRbits.PCIF))))) {

        i2c_command->in_progress = 0;

        DMAnCON0bits.EN = 0;

        if (i2c_command->operation == I2C_OPERATION_WRITE_THEN_READ) {
            i2c_command->operation = I2C_OPERATION_READ;
        } else {
            i2c_command->state = I2C_STATE_SUCCESS;

            if (i2c_command->callback != NULL) {
                i2c_command = i2c_command->callback(i2c_command);
            } else {
                i2c_command = NULL;
            }
        }
    }

    if (i2c_command == NULL) {
        i2c_command = i2c_queue_head;

        if (i2c_command != NULL) {
            i2c_queue_head = i2c_command->next_cmd;
        }

        if (i2c_queue_head == NULL) {
            i2c_queue_tail = NULL;
        }
    }

    if (i2c_command != NULL && !i2c_command->in_progress) {
        i2c_command->state = I2C_STATE_IN_PROGRESS;

        DMAnCON1bits.SMR = 0b00;

        I2C1STAT1bits.CLRBF = 1;
        I2C1PIRbits.PCIF = 0;
        I2C1PIRbits.SCIF = 0;
        I2C1PIRbits.RSCIF = 0;

        switch (i2c_command->operation) {
            case I2C_OPERATION_WRITE_THEN_READ:
            case I2C_OPERATION_WRITE:
                DMAnCON1bits.SMODE = 0b01;
                DMAnCON1bits.DMODE = 0b00;

                I2C1CON1bits.ACKCNT = 0;

                I2C1ADB1 = (i2c_command->addr << 1) & 0xfe;

                I2C1CNTH = (i2c_command->write_size >> 8) & 0xff;
                I2C1CNTL = i2c_command->write_size & 0xff;

                DMAnSSZH = I2C1CNTH;
                DMAnSSZL = I2C1CNTL;
                DMAnDSZ = 1;

                DMAnSSA = (volatile uint24_t) i2c_command->buffer;
                DMAnDSA = (volatile unsigned short) &I2C1TXB;

                DMAnCON1bits.SSTP = 1;
                DMAnCON1bits.DSTP = 0;

                DMAnSIRQ = 0x39;

                I2C1CON0bits.S = 1;
                break;

            case I2C_OPERATION_READ:
                DMAnCON1bits.SMODE = 0b00;
                DMAnCON1bits.DMODE = 0b01;

                I2C1CON1bits.ACKCNT = 1;

                I2C1ADB1 = ((uint8_t) (i2c_command->addr << 1)) | 0x01;

                I2C1CNTH = (i2c_command->read_size >> 8) & 0xff;
                I2C1CNTL = i2c_command->read_size & 0xff;

                DMAnSSZ = 1;
                DMAnDSZH = I2C1CNTH;
                DMAnDSZL = I2C1CNTL;

                DMAnSSA = (volatile uint24_t) & I2C1RXB;
                DMAnDSA = (volatile unsigned short) i2c_command->buffer;

                DMAnCON1bits.SSTP = 0;
                DMAnCON1bits.DSTP = 1;

                DMAnSIRQ = 0x38;

                I2C1CON0bits.S = 1;
                break;
        }

        PIR2bits.DMA1DCNTIF = 0;

        DMAnCON0bits.SIRQEN = 1;
        DMAnCON0bits.EN = 1;

        i2c_command->in_progress = 1;
    }
}

void i2c_enqueue(i2c_command_t *c) {
    c->next_cmd = NULL;

    if (i2c_queue_tail == NULL) {
        i2c_queue_head = c;
    } else {

        i2c_queue_tail->next_cmd = c;
    }

    i2c_queue_tail = c;
}

i2c_command_t *i2c_unused_callback(i2c_command_t *cmd) {
    return NULL;
}
