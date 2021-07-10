#ifndef LCD_H
#define	LCD_H

void lcd_initialize(void);
void lcd_set_brightness(uint8_t bri);
void lcd_set_contrast(uint8_t cont);
void lcd_service(void);
void lcd_sync(void);
void lcd_update(uint8_t line, uint8_t col, uint8_t size, uint8_t *data);
void lcd_clear(void);
void lcd_default(void);
void lcd_update_big(uint8_t pos, uint8_t ch);

#endif	/* LCD_H */
