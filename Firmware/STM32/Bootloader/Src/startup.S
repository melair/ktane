.syntax unified
.cpu cortex-m0plus
.thumb

.section .isr_vector,"a",%progbits
.global Bootloader_Vectors
.type Bootloader_Vectors, %object
Bootloader_Vectors:
    .word __ram_end
    .word Bootloader_Reset_Handler
    .rept 46
    .word Bootloader_Default_Handler
    .endr
.size Bootloader_Vectors, .-Bootloader_Vectors
.global __vector_table_size
.set __vector_table_size, .-Bootloader_Vectors

.section .text.Bootloader_Reset_Handler,"ax",%progbits
.global Bootloader_Reset_Handler
.type Bootloader_Reset_Handler, %function
.thumb_func
Bootloader_Reset_Handler:
    /* No writable globals, C runtime, clock setup, or peripheral setup. */
    cpsid i
    ldr r0, =Bootloader_Vectors
    ldr r1, =0xE000ED08 /* SCB->VTOR */
    str r0, [r1]
    dsb
    isb
    ldr r0, =__ram_end
    mov sp, r0
    bl Bootloader_Main
    b Bootloader_Default_Handler
.size Bootloader_Reset_Handler, .-Bootloader_Reset_Handler

.section .text.Bootloader_EnterApplication,"ax",%progbits
.global Bootloader_EnterApplication
.type Bootloader_EnterApplication, %function
.thumb_func
Bootloader_EnterApplication:
    /* r0 = application MSP; r1 = Thumb reset vector. No stack use below. */
    movs r2, #0
    msr control, r2
    isb
    msr msp, r0
    dsb
    isb
    bx r1
.size Bootloader_EnterApplication, .-Bootloader_EnterApplication

.section .text.Bootloader_Default_Handler,"ax",%progbits
.global Bootloader_Default_Handler
.type Bootloader_Default_Handler, %function
.thumb_func
Bootloader_Default_Handler:
    cpsid i
1:  b 1b
.size Bootloader_Default_Handler, .-Bootloader_Default_Handler

.section .note.GNU-stack,"",%progbits
