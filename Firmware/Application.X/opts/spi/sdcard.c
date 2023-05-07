#include <xc.h>
#include "fat.h"
#include "sdcard.h"
#include "spi_data.h"
#include "../../opts.h"
#include "../../hal/spi.h"
#include "../../hal/pins.h"

/* Local function prototypes. */
spi_command_t *sdcard_init_callback(spi_command_t *cmd);
void sdcard_transaction_setup(sd_transaction_t *t, opt_data_t *opt, uint8_t cmd, uint32_t data, uint8_t *resp_buffer);

void sdcard_initialise(opt_data_t *opt) {
    opt->spi.sdcard.spi.device.clk_pin = opt->spi.pins.sclk;
    opt->spi.sdcard.spi.device.mosi_pin = opt->spi.pins.mosi;
    opt->spi.sdcard.spi.device.miso_pin = opt->spi.pins.miso;
    opt->spi.sdcard.spi.device.cs_pin = KPIN_NONE;
    opt->spi.sdcard.spi.device.baud = SPI_BAUD_125_KHZ;
    opt->spi.sdcard.spi.device.cke = true;

    opt->spi.sdcard.flags.init_stage = 0;
    opt->spi.sdcard.init_transaction.spi_cmd.device = &opt->spi.sdcard.spi.device;
    opt->spi.sdcard.init_transaction.spi_cmd.callback = sdcard_init_callback;
    opt->spi.sdcard.init_transaction.spi_cmd.callback_ptr = (void *) opt;

    sdcard_init_callback(&opt->spi.sdcard.init_transaction.spi_cmd);
    spi_enqueue(&opt->spi.sdcard.init_transaction.spi_cmd);

    fat_initialise(opt);
}

#define SDCARD_PREAMBLE_LENGTH 10

spi_command_t *sdcard_init_callback(spi_command_t *cmd) {
    opt_data_t *opt = (opt_data_t *) cmd->callback_ptr;

    switch (opt->spi.sdcard.flags.init_stage) {
        case 0:
            /* SD Card Sync pulses, 80 without CS asserted. */
            cmd->operation = SPI_OPERATION_WRITE;
            cmd->write_size = SDCARD_PREAMBLE_LENGTH;
            cmd->buffer = &opt->spi.sdcard.init_transaction.outbound_data[0];

            for (uint8_t i = 0; i < SDCARD_PREAMBLE_LENGTH; i++) {
                cmd->buffer[i] = 0xff;
            }

            opt->spi.sdcard.flags.init_stage++;

            break;

        case 1:
            sdcard_transaction_setup(&opt->spi.sdcard.init_transaction, opt, 0, 0x00000000, &opt->spi.sdcard.init_inbound_data[0]);
            sdcard_transaction_callback(&opt->spi.sdcard.init_transaction);

            opt->spi.sdcard.flags.init_stage++;
            break;

        case 2:
            switch (sdcard_transaction_callback(&opt->spi.sdcard.init_transaction)) {
                case SDCARD_CMD_COMPLETE:
                    opt->spi.sdcard.flags.init_stage++;
                    break;
            }
            break;

        case 3:
            sdcard_transaction_setup(&opt->spi.sdcard.init_transaction, opt, 8, 0x000001aa, &opt->spi.sdcard.init_inbound_data[0]);
            sdcard_transaction_callback(&opt->spi.sdcard.init_transaction);

            opt->spi.sdcard.flags.init_stage++;
            break;

        case 4:
            switch (sdcard_transaction_callback(&opt->spi.sdcard.init_transaction)) {
                case SDCARD_CMD_ERROR:
                    return NULL;
                case SDCARD_CMD_COMPLETE:
                    if ((opt->spi.sdcard.init_inbound_data[2] & 0x0f) != 0x01) {
                        /* Check if SD card reports that 2.7V-3.6V is acceptable. */
                        return NULL;
                    }

                    opt->spi.sdcard.flags.init_stage++;
                    break;
            }
            break;

        case 5:
            sdcard_transaction_setup(&opt->spi.sdcard.init_transaction, opt, 58, 0x00000000, &opt->spi.sdcard.init_inbound_data[0]);
            sdcard_transaction_callback(&opt->spi.sdcard.init_transaction);

            opt->spi.sdcard.flags.init_stage++;
            break;

        case 6:
            switch (sdcard_transaction_callback(&opt->spi.sdcard.init_transaction)) {
                case SDCARD_CMD_ERROR:
                    return NULL;
                case SDCARD_CMD_COMPLETE:
                    /* TODO: Validate voltage ranges.*/

                    opt->spi.sdcard.flags.init_stage++;
                    break;
            }
            break;

        case 7:
            sdcard_transaction_setup(&opt->spi.sdcard.init_transaction, opt, 55, 0x00000000, &opt->spi.sdcard.init_inbound_data[0]);
            sdcard_transaction_callback(&opt->spi.sdcard.init_transaction);

            opt->spi.sdcard.flags.init_stage++;
            break;

        case 8:
            switch (sdcard_transaction_callback(&opt->spi.sdcard.init_transaction)) {
                case SDCARD_CMD_ERROR:
                    return NULL;
                case SDCARD_CMD_COMPLETE:
                    opt->spi.sdcard.flags.init_stage++;
                    break;
            }
            break;

        case 9:
            sdcard_transaction_setup(&opt->spi.sdcard.init_transaction, opt, 41, 0x40000000, &opt->spi.sdcard.init_inbound_data[0]);
            sdcard_transaction_callback(&opt->spi.sdcard.init_transaction);

            opt->spi.sdcard.flags.init_stage++;
            break;

        case 10:
            switch (sdcard_transaction_callback(&opt->spi.sdcard.init_transaction)) {
                case SDCARD_CMD_ERROR:
                    return NULL;
                case SDCARD_CMD_COMPLETE:
                    if (opt->spi.sdcard.init_inbound_data[0] != 0x00) {
                        sdcard_transaction_setup(&opt->spi.sdcard.init_transaction, opt, 55, 0x00000000, &opt->spi.sdcard.init_inbound_data[0]);
                        sdcard_transaction_callback(&opt->spi.sdcard.init_transaction);

                        opt->spi.sdcard.flags.init_stage = 8;
                        break;
                    }

                    opt->spi.sdcard.flags.init_stage++;
                    break;
            }
            break;

        case 11:
            sdcard_transaction_setup(&opt->spi.sdcard.init_transaction, opt, 58, 0x00000000, &opt->spi.sdcard.init_inbound_data[0]);
            sdcard_transaction_callback(&opt->spi.sdcard.init_transaction);

            opt->spi.sdcard.flags.init_stage++;
            break;

        case 12:
            switch (sdcard_transaction_callback(&opt->spi.sdcard.init_transaction)) {
                case SDCARD_CMD_ERROR:
                    return NULL;
                case SDCARD_CMD_COMPLETE:
                    opt->spi.sdcard.flags.ready = (opt->spi.sdcard.init_inbound_data[0] & 0x80) == 0x80;
                    opt->spi.sdcard.flags.high_capacity = (opt->spi.sdcard.init_inbound_data[0] & 0x40) == 0x40;
                    opt->spi.sdcard.spi.device.baud = SPI_BAUD_6400_KHZ;

                    fat_scan();
                    return NULL;
            }

            break;

        default:
            return NULL;
    }

    return cmd;
}

void sdcard_transaction_setup(sd_transaction_t *t, opt_data_t *opt, uint8_t cmd, uint32_t data, uint8_t *resp_buffer) {
    t->state = 0;
    t->outbound_data[0] = 0x40 | cmd;
    t->outbound_data[1] = (data >> 24) & 0xff;
    t->outbound_data[2] = (data >> 16) & 0xff;
    t->outbound_data[3] = (data >> 8) & 0xff;
    t->outbound_data[4] = data & 0xff;
    t->inbound_data = resp_buffer;
    t->spi_cmd.device = &opt->spi.sdcard.spi.device;
    t->spi_cs = opt->spi.pins.sd_cs;

    switch (cmd) {
        case 0:
            t->outbound_data[5] = 0x95;
            break;
        case 8:
            t->outbound_data[5] = 0x87;
            break;
        case 58:
            t->outbound_data[5] = 0xfd;
            break;
        default:
            t->outbound_data[5] = 0x01;
            break;
    }

    t->result = SDCARD_CMD_COMPLETE;

    switch (cmd) {
        case 0: /* CMD0 */
        case 55: /* CMD55 */
        case 41: /* ACMD41 */
            t->response_type = 1;
            break;
        case 8: /* CMD8 */
            t->response_type = 7;
            break;
        case 58: /* CMD58 */
            t->response_type = 3;
            break;
        case 17:
            t->response_type = 15;
            break;
    }

}

uint8_t sdcard_transaction_callback(sd_transaction_t *t) {
    switch (t->state) {
            /* Clock 8 bits out without CS, for state machine. */
        case 0:
            kpin_write(t->spi_cs, true);

            t->spi_cmd.operation = SPI_OPERATION_WRITE;
            t->spi_cmd.write_size = 1;
            t->spi_cmd.buffer = &t->scratch_data[0];
            t->spi_cmd.buffer[0] = 0xff;

            t->state++;
            break;

            /* Clock 8 bits with CS, for state machine. */
        case 1:
            kpin_write(t->spi_cs, false);

            t->spi_cmd.operation = SPI_OPERATION_WRITE;
            t->spi_cmd.write_size = 1;
            t->spi_cmd.buffer = &t->scratch_data[0];
            t->spi_cmd.buffer[0] = 0xff;

            t->state++;
            break;

            /* Write command out. */
        case 2:
            t->spi_cmd.operation = SPI_OPERATION_WRITE;
            t->spi_cmd.write_size = 6;
            t->spi_cmd.buffer = &t->outbound_data[0];

            t->state++;
            break;

            /* Read R1. */
        case 3:
            t->spi_cmd.operation = SPI_OPERATION_READ;
            t->spi_cmd.read_size = 1;
            t->spi_cmd.buffer = t->inbound_data;

            t->state++;
            break;

            /* Parse R1 response, read R3/R7. */
        case 4:
            if (t->spi_cmd.buffer[0] == 0xff) {
                /* Repeat the SPI command from stage 3, read R1. */
                break;
            } else if ((t->spi_cmd.buffer[0] & 0xfe) != 0x00) {
                t->result = SDCARD_CMD_ERROR;

                t->spi_cmd.operation = SPI_OPERATION_WRITE;
                t->spi_cmd.write_size = 1;
                t->spi_cmd.buffer = &t->scratch_data[0];
                t->spi_cmd.buffer[0] = 0xff;

                t->state += 2;
                break;
            }

            switch (t->response_type) {
                case 1:
                    /* Write CS low tail. */
                    t->spi_cmd.operation = SPI_OPERATION_WRITE;
                    t->spi_cmd.write_size = 1;
                    t->spi_cmd.buffer = &t->scratch_data[0];
                    t->spi_cmd.buffer[0] = 0xff;

                    t->state += 2;
                    break;

                case 3:
                case 7:
                    t->spi_cmd.operation = SPI_OPERATION_READ;
                    t->spi_cmd.read_size = 4;
                    t->spi_cmd.buffer = t->inbound_data;

                    t->state++;
                    break;

                    /* Not R1/R3/R7, block read, 512 bytes. */
                case 15:
                    t->spi_cmd.operation = SPI_OPERATION_READ;
                    t->spi_cmd.read_size = 1;
                    t->spi_cmd.buffer = &t->scratch_data[0];

                    t->state = 8;
                    break;
            }

            break;

            /* R3 or R7 response, received. Write 8 bits for state machine with CS. */
        case 5:
            t->spi_cmd.operation = SPI_OPERATION_WRITE;
            t->spi_cmd.write_size = 1;
            t->spi_cmd.buffer = &t->scratch_data[0];
            t->spi_cmd.buffer[0] = 0xff;

            t->state++;
            break;

            /* Clock 8 bits out without CS, for state machine. */
        case 6:
            kpin_write(t->spi_cs, true);
            t->spi_cmd.operation = SPI_OPERATION_WRITE;
            t->spi_cmd.write_size = 1;
            t->spi_cmd.buffer = &t->scratch_data[0];
            t->spi_cmd.buffer[0] = 0xff;

            t->state++;
            break;

            /* Return the result. */
        case 7:
            return t->result;

            /*** BLOCK READ STAGES ***/

            /* Read the block. */
        case 8:
            if (t->scratch_data[0] != 0xfe) {
                /* Repeat last read, SD card not ready. */
                break;
            }

            t->spi_cmd.operation = SPI_OPERATION_READ;
            t->spi_cmd.read_size = SDCARD_SECTOR_SIZE;
            t->spi_cmd.buffer = t->inbound_data;

            t->state++;
            break;

            /* Read the CRC16. */
        case 9:
            t->spi_cmd.operation = SPI_OPERATION_READ;
            t->spi_cmd.read_size = 2;
            t->spi_cmd.buffer = &t->scratch_data[0];

            t->state = 5;
            break;

    }

    return SDCARD_CMD_INPROGRESS;
}

bool sdcard_is_ready(opt_data_t *opt) {
    return opt->spi.sdcard.flags.ready;
}

void sdcard_transaction_read_block(sd_transaction_t *t, opt_data_t *opt, uint8_t *buffer, uint32_t block) {
    if (!opt->spi.sdcard.flags.high_capacity) {
        block *= 512;
    }

    sdcard_transaction_setup(t, opt, 17, block, buffer);
}