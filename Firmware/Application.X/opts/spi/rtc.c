#include <xc.h>
#include "spi_data.h"
#include "../../opts.h"
#include "../../hal/spi.h"

/* Local function prototypes. */
spi_command_t *rtc_init_callback(spi_command_t *cmd);
spi_command_t *rtc_service_callback(spi_command_t *cmd);

void rtc_initialise(opt_data_t *opt) {   
    opt->spi.rtc.device.clk_pin = opt->spi.pins.sclk;
    opt->spi.rtc.device.mosi_pin = opt->spi.pins.mosi;
    opt->spi.rtc.device.miso_pin = opt->spi.pins.miso;
    opt->spi.rtc.device.cs_pin = opt->spi.pins.rtc_cs;
    opt->spi.rtc.device.baud = SPI_BAUD_125_KHZ;
    opt->spi.rtc.device.cke = false;

    opt->spi.rtc.one.device = &opt->spi.rtc.device;   
    opt->spi.rtc.two.device = &opt->spi.rtc.device;
    
    opt->spi.rtc.one.buffer = &opt->spi.rtc.one_buffer[0];
    opt->spi.rtc.two.buffer = &opt->spi.rtc.two_buffer[0];
    
    opt->spi.rtc.flags.ready = false;    
    
    opt->spi.rtc.one.buffer[0] = 0x88; // Start writing at 0x08
    opt->spi.rtc.one.buffer[1] = 0x00; // Alarm Tens/Hundreds to 0
    opt->spi.rtc.one.buffer[2] = 0x80; // Alarm Seconds, masked.
    opt->spi.rtc.one.buffer[3] = 0x80; // Alarm Minutes, masked.
    opt->spi.rtc.one.buffer[4] = 0x80; // Alarm Hours, masked.
    opt->spi.rtc.one.buffer[5] = 0x80; // Alarm Date/Day, masked.
    opt->spi.rtc.one.buffer[6] = 0x05; // Control, /INT and Alarm Interrupts
    opt->spi.rtc.one.buffer[7] = 0x00; // Status, clear Alarm Interrupt
    opt->spi.rtc.one.write_size = 8;
    opt->spi.rtc.one.read_size = 0;
    opt->spi.rtc.one.operation = SPI_OPERATION_WRITE;
    opt->spi.rtc.one.callback = rtc_init_callback;
    opt->spi.rtc.one.callback_ptr = opt;     

    spi_enqueue(&opt->spi.rtc.one);
}


spi_command_t *rtc_init_callback(spi_command_t *cmd) {
    opt_data_t *opt = (opt_data_t *) cmd->callback_ptr;
    opt->spi.rtc.flags.ready = true;    
    return NULL;    
}

void rtc_service(opt_data_t *opt) {
    if (!kpin_read(opt->spi.pins.i_r_q) && opt->spi.rtc.flags.ready && !opt->spi.rtc.flags.in_irq) {
        opt->spi.rtc.flags.in_irq = true;
               
        opt->spi.rtc.one.buffer[0] = 0x8e; // Start writing at 0x0e
        opt->spi.rtc.one.buffer[1] = 0x00;
        opt->spi.rtc.one.write_size = 2;
        opt->spi.rtc.one.read_size = 0;
        opt->spi.rtc.one.operation = SPI_OPERATION_WRITE;
        opt->spi.rtc.one.callback = NULL;
        opt->spi.rtc.one.callback_ptr = NULL;
        
        spi_enqueue(&opt->spi.rtc.one);
        
        opt->spi.rtc.two.buffer[0] = 0x01; // Start reading at 0x01
        opt->spi.rtc.two.write_size = 1;
        opt->spi.rtc.two.read_size = 7;
        opt->spi.rtc.two.operation = SPI_OPERATION_WRITE_THEN_READ;
        opt->spi.rtc.two.callback = rtc_service_callback;
        opt->spi.rtc.two.callback_ptr = opt;   
        
        spi_enqueue(&opt->spi.rtc.two);                               
    }
}

spi_command_t *rtc_service_callback(spi_command_t *cmd) {
    opt_data_t *opt = (opt_data_t *) cmd->callback_ptr;
    opt->spi.rtc.flags.in_irq = false;
    
    uint8_t seconds = (((opt->spi.rtc.two_buffer[0] & 0b01110000) >> 4) * 10) + (opt->spi.rtc.two_buffer[0] & 0b00001111);
    uint8_t minutes = (((opt->spi.rtc.two_buffer[1] & 0b01110000) >> 4) * 10) + (opt->spi.rtc.two_buffer[1] & 0b00001111);
    uint8_t hours = (((opt->spi.rtc.two_buffer[2] & 0b00110000) >> 4) * 10) + (opt->spi.rtc.two_buffer[2] & 0b00001111);
    
    uint8_t day = (opt->spi.rtc.two_buffer[3] & 0b00000111);

    uint8_t date = (((opt->spi.rtc.two_buffer[4] & 0b00110000) >> 4) * 10) + (opt->spi.rtc.two_buffer[4] & 0b00001111);
    uint8_t month = (((opt->spi.rtc.two_buffer[5] & 0b00010000) >> 4) * 10) + (opt->spi.rtc.two_buffer[5] & 0b00001111);
    uint8_t year = (((opt->spi.rtc.two_buffer[6] & 0b11110000) >> 4) * 10) + (opt->spi.rtc.two_buffer[6] & 0b00001111);
        
    return NULL;    
}
