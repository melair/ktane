#include "dma.h"
#include "argb.h"
#include "argb_internal.h"
#include "../utils/fsm.h"
#include "pin.h"
#include <xc.h>

typedef struct {
  fsm_t fsm;
  argb_led_t *buffer;
  uint8_t buffer_len;
} argb_t;

argb_led_t argb_default_buffer[ARGB_DEFAULT_BUFFER_SIZE];

argb_t argb = { .buffer_len = 0 };

void argb_init(pin_t out, bool negate) {
    pin_config(out, OUTPUT, 0);

    /** PWM for 1.6MHz clock generation. **/
    /* Initialise PWM to FOSC (64MHz). */
    ARGB_PWM_CLK = 0b0010;
    /* Set Period to 40, i.e. 1.6MHz. */
    ARGB_PWM_PR = 39; // (n+1 calculation in silicon)
    /* Set slice one duty cycle to 60%. */
    ARGB_PWM_S1P1 = 24;
    /* Turn on PWM. */
    ARGB_PWM_EN = 1;

    /** CLC for buffering PWM to SPI as clock source. */
    /* Select CLC. */
    CLCSELECT = ARGB_CLC_PWM_SPI_PERIPHERAL;
    /* Clear CLC. */
    CLCnCON = 0x00;
    /* Link CLC data register 1 to PWMxS1P1. */
    CLCnSEL0 = CLC_PWM_BASE + ((ARGB_PWM_PERIPHERAL - 1) * 2);
    /* Clear data registers 2, 3 and 4. */
    CLCnSEL1 = 0x00;
    CLCnSEL2 = 0x00;
    CLCnSEL3 = 0x00;
    /* Set mode to AND-OR. */
    CLCnCONbits.MODE = 0b000;
    /* Clear Gate/Data mappings. */
    CLCnGLS0 = 0x00;
    CLCnGLS1 = 0x00;
    CLCnGLS2 = 0x00;
    CLCnGLS3 = 0x00;
    /* Set Gateway 1 to select data register 1. */
    CLCnGLS0bits.G1D1T = 1;
    /* Clear polarity register.*/
    CLCnPOL = 0x00;
    /* Invert gate 2, providing constant high. */
    CLCnPOLbits.G2POL = 1;
    /* Enable CLC. */
    CLCnCONbits.EN = 1;

    /** SPI for output, use above as clock source. */
    /* Select TX only. */
    ARGB_SPI_CON_TXR = 1;
    /* Set to master mode. */
    ARGB_SPI_CON_MST = 1;
    /* Set BMODE to 1, counting in bytes. */
    ARGB_SPI_CON_BMODE = 1;
    /* Set SPI clock source to CLC_PWM_SPI_OUT. */
    ARGB_SPI_CLK = SPI_CLK_CLC_BASE + ARGB_CLC_PWM_SPI_PERIPHERAL;
    /* Set SPI baud to 0, resulting in 800kHz SPI. */
    ARGB_SPI_BAUD = 0;
    /* Enable SPI. */
    ARGB_SPI_CON_EN = 1;

    /** CLC for generating WS2812 signal. */
    /* This uses the logic algorithm from http://ww1.microchip.com/downloads/en/appnotes/00001606a.pdf,
     * it is not intuitive to casual viewers.
     *
     * CLC Configured As: (SCK & nSDO & PWM) || (SCK & SDO)
     * Functionally Equal To: SCK && (PWM || SDO) */
    CLCSELECT = ARGB_CLC_SPI_OUT_PERIPHERAL;
    /* Clear polarity register.*/
    CLCnPOL = 0x00;
    /* Link CLC data register 1 to PWMxS1P1. */
    CLCnSEL0 = CLC_PWM_BASE + ((ARGB_PWM_PERIPHERAL - 1) * 2);
    /* Link CLC data register 2 to SPI1 CLK. */ // TODO
    CLCnSEL1 = CLC_SPI_SCK_BASE + (ARGB_CLC_PWM_SPI_PERIPHERAL * 3);
    /* Link CLC data register 3 to SPI1 SDO. */ // TODO
    CLCnSEL2 = CLC_SPI_SDO_BASE + (ARGB_CLC_PWM_SPI_PERIPHERAL * 3);
    /* Clear data registers 4. */
    CLCnSEL3 = 0x00;
    /* Set mode to AND-OR. */
    CLCnCONbits.MODE = 0b000;
    /* Clear Gate/Data mappings. */
    CLCnGLS0 = 0x05;
    // G2D2N disabled; G2D1N disabled; G2D4N disabled; G2D3N enabled; G2D2T disabled; G2D1T disabled; G2D4T disabled; G2D3T disabled;
    CLCnGLS1 = 0x10;
    // G3D1N disabled; G3D2N disabled; G3D3N disabled; G3D4N disabled; G3D1T disabled; G3D2T enabled; G3D3T disabled; G3D4T disabled;
    CLCnGLS2 = 0x08;
    // G4D1N disabled; G4D2N disabled; G4D3N disabled; G4D4N disabled; G4D1T disabled; G4D2T disabled; G4D3T enabled; G4D4N disabled;
    CLCnGLS3 = 0x20;
    /* Set polarity of gate 1. */
    CLCnPOLbits.G1POL = 1;
    /* Set polarity of output. */
    if (negate) {
        CLCnPOLbits.POL = 1;
    } else {
        CLCnPOLbits.POL = 0; 
    }
    /* Enable CLC. */
    CLCnCONbits.EN = 1;

    *pin_to_pps(out) = 0x01 + ARGB_CLC_SPI_OUT_PERIPHERAL;

    /* Select DMA. */
    DMA_SELECT_BEGIN(ARGB_DMA);
    /* Ensure DMA module is disabled. */
    DMAnCON0bits.EN = 0;
    /* Clear DMA. */
    DMAnCON0 = 0;
    /* Set destination size to be 1. */
    DMAnDSZ = 1;
    /* Set destination to be SPI. */
    DMAnDSA = (volatile unsigned short) &ARGB_SPI_TXB;
    /* Set source address to general purpose register space. */
    DMAnCON1bits.SMR = 0;
    /* Increment source address after every transfer. */
    DMAnCON1bits.SMODE = 1;
    /* Keep destination address static after every transfer. */
    DMAnCON1bits.DMODE = 0;
    /* Set clearing of SIREQEN bit when source counter is reloaded, don't when
     * destination counter is reloaded. */
    DMAnCON1bits.SSTP = 1;
    DMAnCON1bits.DSTP = 0;
    /* Set start and abort IRQ triggers, SPIxTX and None, respectively. */
    DMAnSIRQ = SPITXVECTOR;
    DMAnAIRQ = 0x00;
    /* Prevent hardware triggers starting DMA transfer. */
    DMAnCON0bits.SIRQEN = 0;
    /* Request interrupts for completion of DMA. */
    dma_inte_dcnt(ARGB_DMA, true);

    DMA_SELECT_END();

    /* Set the default buffer to ensure that DMA source is correct. */
    argb_set_buffer(&argb_default_buffer[0], ARGB_DEFAULT_BUFFER_SIZE);
}

void argb_service(void) {

}

void argb_interrupt(void) {

}

void argb_set_buffer(argb_led_t *buffer, uint8_t len) {
    uint8_t copy_size = len;
    if (argb.buffer_len < copy_size) {
        copy_size = argb.buffer_len;
    }

    for (uint8_t i = 0; i < copy_size; i++) {
        buffer[i].R = argb.buffer[i].R;
        buffer[i].G = argb.buffer[i].G;
        buffer[i].B = argb.buffer[i].B;
    }

    argb.buffer = buffer;
    argb.buffer_len = len;

    DMA_SELECT_BEGIN(ARGB_DMA);
    DMAnSSZ = sizeof (argb_t) * argb.buffer_len;
    DMAnSSA = argb.buffer;
    DMA_SELECT_END();
}

void argb_set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (idx <= argb.buffer_len) {
        argb.buffer[idx].R = r;
        argb.buffer[idx].G = g;
        argb.buffer[idx].B = b;
    }
}

void argb_update(void) {

}