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
#include "can.h"
#include "edgework.h"
#include "firmware.h"
#include "game.h"
#include "interrupt.h"
#include "lcd.h"
#include "mode.h"
#include "module.h"
#include "nvm.h"
#include "peripherals/timer/segment.h"
#include "protocol_module.h"
#include "serial.h"
#include "status.h"
#include "tick.h"
#include "rng.h"

#pragma config JTAGEN = OFF             // Disable JTAG Boundary Scan

#pragma config FEXTOSC = OFF            // External Oscillator Selection (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_64MHZ  // Reset Oscillator Selection (HFINTOSC with HFFRQ = 64 MHz and CDIV = 1:1)
#pragma config MVECEN = ON             // Multi-vector enable bit (Interrupt contoller does use vector table to prioritze interrupts)

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
void oscillator_initialise(void);
void pmd_initialise(void);

/**
 * Main function, initialise and main loop.
 */
void main(void) {
    /* Drive everything unused to low. */
    safe_unused_pins();
    
    /* Reconfigure system arbiter. */
    arbiter_initialise();
    
    /* Configure sleep behaviour, put CPU in IDLE. */
    CPUDOZEbits.IDLEN = 1;
    
    /* Disable unused peripherals. */
    pmd_initialise();
    
    /* Initialise interrupts. */
    int_initialise();   
          
    /* Initialise firmware. */
    firmware_initialise();
    
    /* Unlock PPS during initialisation. */
    pps_unlock();
                                  
    /* Initialise tick. */
    tick_initialise();
    
    /* Initialise EEPROM, including data migrations. */
    nvm_initialise();    
    
    /* Initialise RNG. */
    rng_initialise();
    
    /* Initialise LCD control. */
    lcd_initialize();
        
    /* Initialise ARGB. */
    argb_initialise();
    
    /* Initialise serial number. */
    serial_initialise();
    
    /* Initialise CAN bus. */
    can_initialise();

    /* Initialise Buzzer. */
    buzzer_initialise();
   
    /* Initialise modules. */
    module_initialise();
       
    /* Initialise the mode. */
    mode_initialise();
    
    /* Initialise game state. */
    game_initialise();
    
    /* Lock PPS during main execution. */
    pps_lock();      
      
    /* Beep on start. */
    buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 100);
    
    /* Set status led to ready. */
    status_set(STATUS_READY);
           
    /* Main loop. */
    while(true) {
        /* Clear watchdog. */
        CLRWDT();    
        
        /* Service the tick. */
        tick_service();
                
        /* Service CAN buffers. */
        can_service();
        
        /* Service module subsystem. */
        module_service();
                
        /* Service the game state. */
        game_service();
        
        /* Update ARGB string if needed. */
        argb_service();    

        /* Service LCD driver. */
        lcd_service();
        
        /* Service buzzer driver. */
        buzzer_service();
        
        /* Service status LED. */
        status_service();
        
        /* Service edgework. */
        edgework_service();
        
        /* Service the mode. */
        mode_service();   
        
        /* Service RNG. */
        rng_service();
        
        /* Put MCU into IDLE mode, no CPU instructions but peripherals continue.
         * 
         * Full SLEEP would be ideal, but is incompatible with the CAN peripheral
         * as SLEEP disables the memory controller. CAN can wake upon start of
         * message but the packet would likely be lost. This behaviour would be
         * suitable for a device that comes on once every few seconds, but for
         * something that needs to wake up at least every 200ms, it wont work.
         * 
         * CPU will wake from the tick timer. This doesn't save much power, but 
         * it prevents functions spinning that don't need to operate faster than 
         * the tick refresh. */
        SLEEP();
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
 * Disable peripherals not used, this saves some power. 
 */
void pmd_initialise(void) {
    PMD0 = 0b01101110; // Keep FOSC, CRC and IOC
    PMD1 = 0b10000000; // Keep Timers
    PMD2 = 0b01111111; // Keep CAN
    PMD3 = 0b11011111; // Keep ADC
    PMD4 = 0b11111111; // Keep Nothing
    PMD5 = 0b00001111; // Keep PWM1-4
    PMD6 = 0b00000001; // Keep UART1-5 and SPI1/1
    PMD7 = 0b11111111; // Keep Nothing
    PMD8 = 0b11111110; // Keep DMA1
}