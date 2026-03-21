//CONFIG1
#pragma config FEXTOSC = OFF     // External Oscillator Mode Selection->Oscillator not enabled
#pragma config RSTOSC = HFINTOSC_64MHZ     // Power-up Default Value for COSC->HFINTOSC with HFFRQ = 64 MHz and CDIV = 1:1

//CONFIG2
#pragma config FCMENP = OFF     // Fail-Safe Clock Monitor Enable - Primary XTAL Enable->Fail-Safe Clock Monitor disabled
#pragma config CLKOUTEN = OFF     // Clock Out Enable->CLKOUT function is disabled
#pragma config FCMEN = OFF     // Fail-Safe Clock Monitor Enable->Fail-Safe Clock Monitor disabled
#pragma config CSWEN = ON     // Clock Switch Enable->Writing to NOSC and NDIV is allowed
#pragma config FCMENS = OFF     // Fail-Safe Clock Monitor Enable - Secondary XTAL Enable->Fail-Safe Clock Monitor Disabled
#pragma config PR1WAY = OFF     // PRLOCKED One-Way Set Enable->PRLOCKED bit can be set and cleared repeatedly

//CONFIG3
#pragma config MVECEN = ON     // Multivector Enable->Multi-vector enabled, Vector table used for interrupts
#pragma config MCLRE = EXTMCLR     // Master Clear (MCLR) Enable->If LVP = 0, MCLR pin is MCLR; If LVP = 1, RE3 pin function is MCLR 
#pragma config BOREN = SBORDIS     // Brown-out Reset Enable->Brown-out Reset enabled , SBOREN bit is ignored
#pragma config PWRTS = PWRT_64     // Power-up Timer Selection->PWRT set at 64ms
#pragma config IVT1WAY = OFF     // IVTLOCK One-Way Set Enable->IVTLOCKED bit can be cleared and set repeatedly
#pragma config LPBOREN = ON     // Low-Power BOR Enable->Low-Power BOR enabled

//CONFIG4
#pragma config XINST = OFF     // Extended Instruction Set Enable->Extended Instruction Set and Indexed Addressing Mode disabled
#pragma config LVP = ON     // Low-Voltage Programming Enable->Low voltage programming enabled. MCLR/VPP pin function is MCLR. MCLRE configuration bit is ignored
#pragma config ZCD = OFF     // ZCD Disable->ZCD module is disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON
#pragma config STVREN = ON     // Stack Overflow/Underflow Reset Enable->Stack full/underflow will cause Reset
#pragma config BORV = VBOR_1P9     // Brown-out Reset Voltage Selection->Brown-out Reset Voltage (VBOR) set to 1.9V
#pragma config PPS1WAY = OFF     // PPSLOCKED One-Way Set Enable->PPSLOCKED bit can be set and cleared repeatedly (subject to the unlock sequence)

//CONFIG5
#pragma config WDTCPS = WDTCPS_8     // WDT Period Select->Divider ratio 1:8192
#pragma config WDTE = NSLEEP     // WDT Operating Mode->WDT enabled while sleep=0, suspended when sleep=1; SWDTEN ignored

//CONFIG6
#pragma config WDTCWS = WDTCWS_6     // WDT Window Select->window always open (100%); no software control; keyed access required
#pragma config WDTCCS = LFINTOSC     // WDT Input Clock Selector->WDT reference clock is the 31.0 kHz LFINTOSC

//CONFIG7
#pragma config SAFEN = OFF     // Storage Area Flash (SAF) Enable->SAF disabled
#pragma config BBEN = OFF     // Boot Block Enable->Boot block disabled
#pragma config BBSIZE = BBSIZE_512     // Boot Block Size Selection->Boot Block size is 512 words

//CONFIG8
#pragma config WRTB = OFF     // Boot Block Write Protection->Boot Block not Write protected
#pragma config WRTC = OFF     // Configuration Register Write Protection->Configuration registers not Write protected
#pragma config WRTD = OFF     // Data EEPROM Write Protection->Data EEPROM not Write protected
#pragma config WRTAPP = OFF     // Application Block Write Protection->Application Block not write protected
#pragma config WRTSAF = OFF     // Storage Area Flash (SAF) Write Protection->SAF not Write Protected

//CONFIG9
#pragma config CP = OFF     // User Program Flash Memory and Data EEPROM Code Protection->PFM and Data EEPROM code protection disabled