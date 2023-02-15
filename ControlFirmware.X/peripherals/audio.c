#include <xc.h>
#include <stdbool.h>
#include "audio.h"

/*
 * The audio peripheral can only operate in PortA as the DAC can't be routed.
 * 
 * Audio must be formatted as unsigned 8-bit at 16000 Samples/sec.
 */

/**
 * Initialise the audio module, this uses the Fixed Voltage Reference peripheral
 * to provide the positive voltage rail for the DAC. Sets DAC maximum voltage
 * to 1.024v.
 */
void audio_initialise(void) {    
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
    /* Switch on timer. */
    T6CONbits.ON = 1;
    
    /* Configure DMA. */
    DMASELECT = 1;
    
    /* Reset DMA2. */
    DMAnCON0 = 0;

    /* Set source and destination address of data. */
    DMAnDSA = &DAC1DATL;

    /* Set source address to general purpose register space. */
    DMAnCON1bits.SMR = 0b00;

    /* Increment source address after every transfer. */
    DMAnCON1bits.SMODE = 0b01;
    /* Keep destination address static after every transfer. */
    DMAnCON1bits.DMODE = 0b00;

    /* Set source and destination sizes, source to the size of the buffer. */
    DMAnDSZ = 1;

    /* Set clearing of SIREQEN bit when source counter is reloaded, don't when
     * destination counter is reloaded. */
    DMAnCON1bits.SSTP = 1;
    DMAnCON1bits.DSTP = 0;

    /* Set start and abort IRQ triggers, Timer6 and None, respectively. */
    DMAnSIRQ = 0x7b;
    DMAnAIRQ = 0x00;

    /* Prevent hardware triggers starting DMA transfer. */
    DMAnCON0bits.SIRQEN = 0;

    /* Enable DMA module. */
    DMAnCON0bits.EN = 1;
}

void audio_service(audio_t *a) {
    /* Start next buffer if last finished. */
    if (PIR6bits.DMA2SCNTIF) {
        /* If buffer isn't ready, then clear it to 0x7f (middle).*/
        if (!a->buffer[a->next].ready) {
            for (uint16_t i = 0; i < a->size; i++) {
                a->buffer[a->next].ptr[i] = 0x7f;
            }
        }
        
        /* Load next buffer into DMA and activate interrupt triggers again.*/
        DMAnSSA = a->buffer[a->next].ptr;
        DMAnSSZ = a->size;
        DMAnCON0bits.SIRQEN = 1;
        
        /* Mark this buffer not ready, and flip to next. */
        a->buffer[a->next].ready = false;
        a->next = (a->next == 0 ? 1 : 0);        
    }
}

uint8_t *audio_empty_buffer(audio_t *a) {
    return a->buffer[a->next].ptr;
}

void audio_mark_ready(audio_t *a) {
    a->buffer[a->next].ready = true;
}
