/**
 * Main file for KTANE, containing main loop and MCU initialisation functions.
 * 
 * Firmware is designed to be non blocking, there should be no loops to wait
 * for either time, or a register bit to clear/set. The PICs peripherals all
 * support interrupt driven operation.
 * 
 * As a result, code tends to be split between an interrupt for handling things
 * that can be processed very quickly (such as incrementing a value, moving
 * along a buffer), and operations which can take a longer time, or can be
 * called by other features happen in a service loop. Service loops should
 * always endeavour to only perform actions if something has changed.
 */
#include <xc.h>
#include <stdbool.h>
#include "argb.h" 
#include "buzzer.h"
#include "nvm.h"
#include "can.h"
#include "lcd.h"
#include "tick.h"
#include "mode.h"
#include "modules.h"

#pragma config JTAGEN = OFF             // Disable JTAG Boundary Scan

#pragma config FEXTOSC = OFF            // External Oscillator Selection (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_64MHZ  // Reset Oscillator Selection (HFINTOSC with HFFRQ = 64 MHz and CDIV = 1:1)
#pragma config MVECEN = OFF             // Multi-vector enable bit (Interrupt contoller does not use vector table to prioritze interrupts)

#pragma config PWRTS = PWRT_16          // Power-up timer selection bits (PWRT set at 16ms)

#ifdef __DEBUG
#pragma config WDTE = OFF
#else
#pragma config WDTE = ON
#pragma config WDTCPS = WDTCPS_11       // WDT Period selection bits (Divider ratio 1:65536)
#pragma config WDTCCS = LFINTOSC        // WDT input clock selector (WDT reference clock is the 31.0 kHz LFINTOSC)
#pragma config WDTCWS = WDTCWS_7        // WDT Window Select bits (window always open (100%); software control; keyed access not required)
#endif

#define _XTAL_FREQ 64000000

/* Function prototypes. */
void safe_unused_pins(void);
void pps_unlock(void);
void pps_lock(void);
void arbiter_initialise(void);
void int_initialise(void);
void oscillator_initialise(void);

/**
 * Main function, initialise and main loop.
 */
void main(void) {
    /* Drive everything unused to low. */
    safe_unused_pins();
    
    /* Reconfigure system arbiter. */
    arbiter_initialise();
    
    /* Unlock PPS during initialisation. */
    pps_unlock();
               
    /* Initialise interrupts. */
    int_initialise();   
                   
    /* Initialise tick. */
    tick_initialise();
    
    /* Initialise EEPROM, including data migrations. */
    nvm_initialise();    
    
    /* Initialise LCD control. */
    lcd_initialize();
        
    /* Initialise ARGB. */
    argb_initialise();

    /* Initialise the mode. */
    mode_initialise();
    
    /* Initialise CAN bus. */
    can_initialise();
        
    /* Initialise modules. */
    modules_initialise();
            
    /* Initialise Buzzer. */
    buzzer_initialise();
    
    /* Lock PPS during main execution. */
    pps_lock();      

    /* Set default LCD text. */
    lcd_default();
    
    /* Beep on start. */
    buzzer_on_timed(10);
    
    /* Main loop. */
    while(true) {
        /* Clear watchdog. */
        CLRWDT();    
        
        /* Update ARGB string if needed. */
        argb_service();    
        
        /* Service CAN buffers. */
        can_service();
        
        /* Service module subsystem. */
        module_service();
        
        /* Service LCD driver. */
        lcd_service();
        
        /* Service buzzer driver. */
        buzzer_service();
        
        /* Service the mode. */
        mode_service();                      
    }
}
                
/** 
 * Safe unused pins.
 */
void safe_unused_pins(void) {
    /* Disable analogue functionality. */
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    ANSELD = 0;
    ANSELE = 0;
    ANSELF = 0;
    
    /* Set all pins to output. */
    TRISA = 0;
    TRISB = 0;
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    TRISF = 0;
            
    /* Drive all pins low. */
    LATA = 0;
    LATB = 0;
    LATC = 0;
    LATD = 0;
    LATE = 0;
    LATF = 0;
}

/**
 * Unlock PPS registers, assumes global interrupts are disabled.
 */
void pps_unlock(void) {
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0;
}

/**
 * Lock PPS registers, assumes global interrupts are disabled.
 */
void pps_lock(void) {
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 1;
}

/**
 * Reconfigure the system arbiter, giving DMA1 and DMA2 priority over the
 * ISR and main CPU.
 */
void arbiter_initialise(void) {
    PRLOCK = 0x55;
    PRLOCK = 0xAA;
    PRLOCKbits.PRLOCKED = 0;
    
    DMA1PR = 0;
    DMA2PR = 1;
    ISRPR = 2;
    MAINPR = 3;
    SCANPR = 4;    
    
    PRLOCK = 0x55;
    PRLOCK = 0xAA;
    PRLOCKbits.PRLOCKED = 1;
}

/**
 * Initialise interrupt control.
 */
void int_initialise(void) {
    /* Enable prioritised interrupts. */
    INTCON0bits.IPEN = 1;
    
    /* Enable global interrupts, high. */
    INTCON0bits.GIEH = 1;
    /* Enable global interrupts, low. */
    INTCON0bits.GIEL = 1;
}

/**
 * Low priority interrupts.
 */
void __interrupt(low_priority) low_priority_routine(void) {
    tick_interrupt();
}

/**
 * High priority interrupts.
 */
void __interrupt(high_priority) high_priority_routine(void) {
}