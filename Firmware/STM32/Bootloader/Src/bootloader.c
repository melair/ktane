#if defined(STM32WB09)
#include "stm32wb09.h"
#else
#include "stm32g070xx.h"
#endif

#include "version/version.h"
#include "board_identity.h"

extern const uint8_t __application_base[];
extern const uint8_t __application_end[];
extern const uint8_t __ram_start[];
extern const uint8_t __ram_end[];

__attribute__((noreturn)) void Bootloader_EnterApplication(uint32_t stack, uint32_t reset);

__attribute__((noreturn, noinline)) void Bootloader_InvalidApplication(void)
{
    __disable_irq();
    for (;;) {
        __NOP();
    }
}

__attribute__((noreturn)) void Bootloader_Main(void)
{
    const uintptr_t base = (uintptr_t)__application_base;
    const volatile version_t *header =
        (const volatile version_t *)(base + VERSION_DESCRIPTOR_OFFSET);
    const uint32_t length = header->image_length;
    const uintptr_t available = (uintptr_t)__application_end - base;

    /* The image begins with the complete vector table; application startup
     * installs its complete vector table using the linked symbol. */
    if (header->magic != VERSION_MAGIC || header->type != VERSION_TYPE_APPLICATION ||
        header->platform != BOARD_PLATFORM || header->hardware != BOARD_HARDWARE ||
        length <= VERSION_CODE_OFFSET || length > available) {
        Bootloader_InvalidApplication();
    }
    const volatile uint32_t *vectors = (const volatile uint32_t *)base;
    const uint32_t stack = vectors[0];
    const uint32_t reset = vectors[1];
    const uint32_t entry = reset & ~UINT32_C(1);

    if ((stack & 7u) != 0u || stack <= (uintptr_t)__ram_start ||
        stack > (uintptr_t)__ram_end || (reset & 1u) == 0u ||
        entry < base + VERSION_CODE_OFFSET || entry >= base + length) {
        Bootloader_InvalidApplication();
    }

    __disable_irq();
    /* These Cortex-M0+ devices have one bank of 32 external interrupts. */
    NVIC->ICER[0] = UINT32_MAX;
    NVIC->ICPR[0] = UINT32_MAX;
    SysTick->CTRL = 0u;
    SysTick->LOAD = 0u;
    SysTick->VAL = 0u;
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    /* Keep the bootloader's complete table active and IRQs masked until the
     * application's first instructions install its own table. */
    __DSB();
    __ISB();
    Bootloader_EnterApplication(stack, reset);
}
