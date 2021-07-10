#ifndef PORTS_H
#define	PORTS_H

#define KPORTA PORTA
#define KPORTB PORTF
#define KPORTC PORTD

#define KPORTAbits PORTAbits
#define KPORTBbits PORTFbits
#define KPORTCbits PORTDbits

#define KTRISA TRISA
#define KTRISB TRISF
#define KTRISC TRISD

#define KTRISAbits TRISAbits
#define KTRISBbits TRISFbits
#define KTRISCbits TRISDbits

#define KLATA LATA
#define KLATB LATF
#define KLATC LATD

#define KLATAbits LATAbits
#define KLATBbits LATFbits
#define KLATCbits LATDbits

#define KODCONA ODCONA
#define KODCONB ODCONF
#define KODCONC ODCOND

#define KODCONAbits ODCONAbits
#define KODCONBbits ODCONFbits
#define KODCONCbits ODCONDbits

#define KWPUA WPUA
#define KWPUB WPUF
#define KWPUC WPUD

#define KWPUAbits WPUAbits
#define KWPUBbits WPUFbits
#define KWPUCbits WPUDbits

typedef uint8_t pin_t;

#define KPIN_A0 ((pin_t) 0b00000000)
#define KPIN_A1 ((pin_t) 0b00000001)
#define KPIN_A2 ((pin_t) 0b00000010)
#define KPIN_A3 ((pin_t) 0b00000011)
#define KPIN_A4 ((pin_t) 0b00000100)
#define KPIN_A5 ((pin_t) 0b00000101)
#define KPIN_A6 ((pin_t) 0b00000110)
#define KPIN_A7 ((pin_t) 0b00000111)

#define KPIN_B0 ((pin_t) 0b00001000)
#define KPIN_B1 ((pin_t) 0b00001001)
#define KPIN_B2 ((pin_t) 0b00001010)
#define KPIN_B3 ((pin_t) 0b00001011)
#define KPIN_B4 ((pin_t) 0b00001100)
#define KPIN_B5 ((pin_t) 0b00001101)
#define KPIN_B6 ((pin_t) 0b00001110)
#define KPIN_B7 ((pin_t) 0b00001111)

#define KPIN_C0 ((pin_t) 0b00010000)
#define KPIN_C1 ((pin_t) 0b00010001)
#define KPIN_C2 ((pin_t) 0b00010010)
#define KPIN_C3 ((pin_t) 0b00010011)
#define KPIN_C4 ((pin_t) 0b00010100)
#define KPIN_C5 ((pin_t) 0b00010101)
#define KPIN_C6 ((pin_t) 0b00010110)
#define KPIN_C7 ((pin_t) 0b00010111)

#define KPIN_NONE 0b11111111

#define PIN_OUTPUT 0
#define PIN_INPUT  1

void kpin_mode(pin_t port, uint8_t mode, bool pullup);
bool kpin_read(pin_t port);
void kpin_write(pin_t port, bool value);

#endif	/* PORTS_H */

