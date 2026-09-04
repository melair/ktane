#ifndef UART_PLATFORM_H
#define UART_PLATFORM_H

#include "uart.h"
#include "sys/gpio.h"

#include <stdbool.h>
#include <stdint.h>

struct UART_Hardware {
    void *uart_instance;
    void *uart_handle;
    uint32_t baud_rate;
    bool swap_rx_tx;

    GPIO_PinDef rx;
    uint32_t rx_alternate;
    GPIO_PinDef tx;
    uint32_t tx_alternate;

    // A non-NULL port enables hardware RS-485 driver-enable control.
    GPIO_PinDef driver_enable;
    uint32_t driver_enable_alternate;
    bool driver_enable_active_low;

    int32_t irq;
    uint32_t irq_priority;

    bool (*configure_clock)(void);
    void (*enable_peripheral_clock)(void);
};

#endif //UART_PLATFORM_H
