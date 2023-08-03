#include <xc.h>
#include <stdbool.h>
#include "audio.h"
#include "audio_data.h"
#include "../../dma.h"
#include "../../malloc.h"

/*
 * The audio peripheral can only operate in PortA as the DAC can't be routed.
 *
 * Audio must be formatted as unsigned 8-bit at 16000 Samples/sec.
 */
uint8_t *audio_buffers;

/**
 * Initialise the audio module, this uses the Fixed Voltage Reference peripheral
 * to provide the positive voltage rail for the DAC. Sets DAC maximum voltage
 * to 1.024v.
 */

void audio_initialise(opt_data_t *opt) {
    audio_buffers = kmalloc(AUDIO_FRAME_SIZE * 2);

    /* Initialise audio buffers. */
    for (uint16_t i = 0; i < AUDIO_FRAME_SIZE * 2; i++) {
        audio_buffers[i] = 0x7f;
    }

    opt->audio.size = AUDIO_FRAME_SIZE;
    opt->audio.buffer = &audio_buffers[0];

    /* Set all signals to output. */
    TRISAbits.TRISA0 = 0;
    TRISAbits.TRISA1 = 0;
    TRISAbits.TRISA2 = 0;
    TRISAbits.TRISA3 = 0;

    /* PWM4 */
    PWM4CLK = 0b00010;
    PWM4CPRE = 0;
    PWM4PR = 0xff;
    PWM4S1P2 = 0x7f;
    PWM4LDS = 0b01101; // DMA3 Destination Count done
    PWM4CONbits.EN = 1;
    PWM4CONbits.LD = 1;

    /* CWG3 */
    CWG3CON0bits.EN = 0;
    CWG3CON0bits.MODE = 0b100;

    CWG3CON1bits.POLA = 0;
    CWG3CON1bits.POLB = 0;
    CWG3CON1bits.POLC = 1;
    CWG3CON1bits.POLD = 1;

    CWG3ISMbits.IS = 0b01011;

    CWG3DBR = 6;
    CWG3DBF = 6;

    CWG3CLKbits.CS = 0;

    CLCSELECT = 0;

    CLCnSEL0 = 0x4C; // CWG3A
    CLCnSEL1 = 0x00;
    CLCnSEL2 = 0x00;
    CLCnSEL3 = 0x00;

    CLCnGLS0 = 0x00;
    CLCnGLS0bits.G1D1T = 1;
    CLCnGLS1 = 0x00;
    CLCnGLS2 = 0x00;
    CLCnGLS3 = 0x00;

    CLCnPOL = 0x00;
    CLCnPOLbits.G2POL = 1;

    CLCnCONbits.MODE = 0b000;
    CLCnCONbits.EN = 1;

    RA3PPS = 0x01; // A, Needs to be via CLC1.
    RA1PPS = 0x12; // B
    RA0PPS = 0x13; // C
    RA2PPS = 0x14; // D

    CWG3CON0bits.EN = 1;

    /* Set up timer for driving the DMA. */
    /* Set timer to use MFINTOSC (32kHz). */
    T6CLKCONbits.CS = 0b00110;
    /* Prescaler of 1:1, resulting in 32kHz. */
    T6CONbits.CKPS = 0b00;
    /* Set period to 1, resulting in interrupting 16kHz. */
    T6PR = 1;

    /* Set up timer for refreshing buffers. */
    /* Set timer to use MFINTOSC (32kHz). */
    T4CLKCONbits.CS = 0b00110;
    /* Prescaler of 1:4, resulting in 8kHz. */
    T4CONbits.CKPS = 0b010;
    /* Set period to 249, resulting in interrupting 32Hz. */
    T4PR = 249;

    /* Configure DMA. */
    DMASELECT = DMA_AUDIO;

    /* Reset DMA2. */
    DMAnCON0 = 0;

    /* Set destination address of data. */
    //    DMAnDSA = (volatile unsigned short) &DAC1DATL;
    DMAnDSA = (volatile unsigned short) &PWM4S1P2L;

    /* Set source address to general purpose register space. */
    DMAnCON1bits.SMR = 0b00;

    /* Increment source address after every transfer. */
    DMAnCON1bits.SMODE = 0b01;
    /* Keep destination address static after every transfer. */
    DMAnCON1bits.DMODE = 0b00;

    /* Set destination sizes, source to the size of the buffer. */
    DMAnDSZ = 1;

    /* Initialise DMA with source buffers. */
    DMAnSSZ = AUDIO_FRAME_SIZE * 2;
    DMAnSSA = (volatile uint24_t) & audio_buffers[0];

    /* Set clearing of SIREQEN bit when source counter is reloaded, don't when
     * destination counter is reloaded. */
    DMAnCON1bits.SSTP = 0;
    DMAnCON1bits.DSTP = 0;

    /* Set start and abort IRQ triggers, Timer6 and None, respectively. */
    DMAnSIRQ = 0x7b;
    DMAnAIRQ = 0x00;

    /* Prevent hardware triggers starting DMA transfer. */
    DMAnCON0bits.SIRQEN = 1;

    /* Enable DMA module. */
    DMAnCON0bits.EN = 1;

    /* Set the next free buffer to be the second, so that the Timer int can
     * flip it. */
    opt->audio.buffer_next = 1;

    /* Switch on timer, which starts audio playback. */
    T6CONbits.ON = 1;
    /* Switch on buffer reload timer. */
    T4CONbits.ON = 1;

    /* Enable interrupts for Timer4 to wake from idle. */
    PIE11bits.TMR4IE = 1;
}

void audio_service(opt_data_t *opt) {
    /* Start next buffer if last finished. */
    if (PIR11bits.TMR4IF) {
        /* Clear Timer 4 interrupt. */
        PIR11bits.TMR4IF = 0;

        opt_audio_t *a = &opt->audio;

        /* Flip to the just played buffer. */
        a->buffer_next = (a->buffer_next == 0 ? 1 : 0);

        if (opt->audio.callback != NULL) {
            opt->audio.callback(a);
        }

    }
}

void audio_register_callback(opt_audio_t *a, void (*callback)(opt_audio_t *a)) {
    a->callback = callback;
}