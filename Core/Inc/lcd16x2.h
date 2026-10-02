#ifndef __LCD16X2_H
#define __LCD16X2_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// Port and pin definition for 4-bit mode
#define LCD16X2_GPIO_CONTROL    GPIOB
#define LCD16X2_GPIO_RS         GPIOB
#define LCD16X2_GPIO_RW         GPIOB
#define LCD16X2_GPIO_EN         GPIOB

#define LCD16X2_GPIO_DATA       GPIOA
#define LCD16X2_GPIO_D4         GPIOA
#define LCD16X2_GPIO_D5         GPIOA
#define LCD16X2_GPIO_D6         GPIOA
#define LCD16X2_GPIO_D7         GPIOA

#define LCD16X2_PIN_RS          GPIO_PIN_12   // Chân PB12 (LCD Pin 4 - RS)
#define LCD16X2_PIN_RW          GPIO_PIN_13   // Chân PB13 (LCD Pin 5 - RW / GND)
#define LCD16X2_PIN_EN          GPIO_PIN_14   // Chân PB14 (LCD Pin 6 - EN)
#define LCD16X2_PIN_D4          GPIO_PIN_4    // Chân PA4 (LCD Pin 11 - D4)
#define LCD16X2_PIN_D5          GPIO_PIN_5    // Chân PA5 (LCD Pin 12 - D5)
#define LCD16X2_PIN_D6          GPIO_PIN_6    // Chân PA6 (LCD Pin 13 - D6)
#define LCD16X2_PIN_D7          GPIO_PIN_7    // Chân PA7 (LCD Pin 14 - D7)

// Commands & Attributes
#define LCD16X2_CLEAR_DISPLAY   0x01
#define LCD16X2_CURSOR_HOME     0x02
#define LCD16X2_DISPLAY_ON_CURSOR_OFF_BLINK_OFF 0x0C

void lcd16x2_init(uint8_t disp_attr);
void lcd16x2_write_command(uint8_t cmd);
void lcd16x2_write_data(uint8_t data);
void lcd16x2_clrscr(void);
void lcd16x2_gotoxy(uint8_t x, uint8_t y);
void lcd16x2_puts(const char *s);

#ifdef __cplusplus
}
#endif

#endif // __LCD16X2_H
