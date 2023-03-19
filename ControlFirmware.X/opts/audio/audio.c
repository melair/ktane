#include <xc.h>
#include <stdbool.h>
#include "audio.h"
#include "audio_data.h"

/*
 * The audio peripheral can only operate in PortA as the DAC can't be routed.
 * 
 * Audio must be formatted as unsigned 8-bit at 16000 Samples/sec.
 */

uint8_t audio_buffers[AUDIO_FRAME_SIZE * 2];

/**
 * Initialise the audio module, this uses the Fixed Voltage Reference peripheral
 * to provide the positive voltage rail for the DAC. Sets DAC maximum voltage
 * to 1.024v.
 */

void audio_initialise(opt_data_t *opt) {
    /* Initialise audio buffers. */
    for (uint16_t i = 0; i < AUDIO_FRAME_SIZE * 2; i++) {
        audio_buffers[i] = 0x7f;
    }
    
    opt->audio.size = AUDIO_FRAME_SIZE;
    opt->audio.buffer = &audio_buffers[0]; 
    
    /* Configure the Voltage Reference to 1.024v. */
    FVRCONbits.CDAFVR = 0b01;
    FVRCONbits.EN = 1;
    
    /* Initialise the DAC. */
    DAC1CONbits.OE = 0b10;
    DAC1CONbits.PSS = 0b10;
    DAC1CONbits.EN = 1;
       
    /* Initial value to 0v. */
    DAC1DATL = 0;    
    
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
    DMASELECT = 2;
    
    /* Reset DMA2. */
    DMAnCON0 = 0;

    /* Set destination address of data. */
    DMAnDSA = &DAC1DATL;

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
    DMAnSSA = &audio_buffers[0];
    
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
}

void audio_service(opt_data_t *opt) {
    opt_audio_t *a = &opt->audio;
    
    /* Start next buffer if last finished. */
    if (PIR11bits.TMR4IF) {
        /* Clear Timer 4 interrupt. */
        PIR11bits.TMR4IF = 0;
                
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