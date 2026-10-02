#include "lcd16x2.h"

static void lcd16x2_toggle_e(void)
{
	LCD16X2_GPIO_EN->BSRR = LCD16X2_PIN_EN;
	delay_us(2);
	LCD16X2_GPIO_EN->BRR = LCD16X2_PIN_EN;
	delay_us(2);
}

static void lcd16x2_write(uint8_t data, uint8_t rs)
{
	LCD16X2_GPIO_RW->BRR = LCD16X2_PIN_RW;
	if (rs)
		LCD16X2_GPIO_RS->BSRR = LCD16X2_PIN_RS;
	else
		LCD16X2_GPIO_RS->BRR = LCD16X2_PIN_RS;

	// High nibble
	LCD16X2_GPIO_D7->BRR = LCD16X2_PIN_D7;
	LCD16X2_GPIO_D6->BRR = LCD16X2_PIN_D6;
	LCD16X2_GPIO_D5->BRR = LCD16X2_PIN_D5;
	LCD16X2_GPIO_D4->BRR = LCD16X2_PIN_D4;
	if (data & 0x80) LCD16X2_GPIO_D7->BSRR = LCD16X2_PIN_D7;
	if (data & 0x40) LCD16X2_GPIO_D6->BSRR = LCD16X2_PIN_D6;
	if (data & 0x20) LCD16X2_GPIO_D5->BSRR = LCD16X2_PIN_D5;
	if (data & 0x10) LCD16X2_GPIO_D4->BSRR = LCD16X2_PIN_D4;
	lcd16x2_toggle_e();

	// Low nibble
	LCD16X2_GPIO_D7->BRR = LCD16X2_PIN_D7;
	LCD16X2_GPIO_D6->BRR = LCD16X2_PIN_D6;
	LCD16X2_GPIO_D5->BRR = LCD16X2_PIN_D5;
	LCD16X2_GPIO_D4->BRR = LCD16X2_PIN_D4;
	if (data & 0x08) LCD16X2_GPIO_D7->BSRR = LCD16X2_PIN_D7;
	if (data & 0x04) LCD16X2_GPIO_D6->BSRR = LCD16X2_PIN_D6;
	if (data & 0x02) LCD16X2_GPIO_D5->BSRR = LCD16X2_PIN_D5;
	if (data & 0x01) LCD16X2_GPIO_D4->BSRR = LCD16X2_PIN_D4;
	lcd16x2_toggle_e();
}

void lcd16x2_write_command(uint8_t cmd)
{
	lcd16x2_write(cmd, 0);
	delay_us((cmd == LCD16X2_CLEAR_DISPLAY || cmd == LCD16X2_CURSOR_HOME) ? 2000 : 50);
}

void lcd16x2_write_data(uint8_t data)
{
	lcd16x2_write(data, 1);
	delay_us(50);
}

void lcd16x2_clrscr(void)
{
	lcd16x2_write_command(LCD16X2_CLEAR_DISPLAY);
}

void lcd16x2_gotoxy(uint8_t x, uint8_t y)
{
	lcd16x2_write_command(0x80 | ((y == 0) ? x : (0x40 + x)));
}

void lcd16x2_puts(const char *s)
{
	while (*s) {
		lcd16x2_write_data((uint8_t)*s++);
	}
}

void lcd16x2_init(uint8_t disp_attr)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = LCD16X2_PIN_RS | LCD16X2_PIN_RW | LCD16X2_PIN_EN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LCD16X2_GPIO_CONTROL, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LCD16X2_PIN_D4 | LCD16X2_PIN_D5 | LCD16X2_PIN_D6 | LCD16X2_PIN_D7;
	HAL_GPIO_Init(LCD16X2_GPIO_DATA, &GPIO_InitStruct);

	LCD16X2_GPIO_CONTROL->BRR = LCD16X2_PIN_RS | LCD16X2_PIN_RW | LCD16X2_PIN_EN;
	delay_us(50000);

	// 8-bit init sequence
	LCD16X2_GPIO_D7->BRR = LCD16X2_PIN_D7;
	LCD16X2_GPIO_D6->BRR = LCD16X2_PIN_D6;
	LCD16X2_GPIO_D5->BSRR = LCD16X2_PIN_D5;
	LCD16X2_GPIO_D4->BSRR = LCD16X2_PIN_D4;
	lcd16x2_toggle_e();
	delay_us(5000);

	lcd16x2_toggle_e();
	delay_us(200);

	lcd16x2_toggle_e();
	delay_us(200);

	// Switch to 4-bit mode (0x20)
	LCD16X2_GPIO_D4->BRR = LCD16X2_PIN_D4;
	lcd16x2_toggle_e();
	delay_us(200);

	// 4-bit, 2 lines, 5x8 font (0x28)
	lcd16x2_write_command(0x28);
	lcd16x2_write_command(0x08); // Display off
	lcd16x2_clrscr();
	lcd16x2_write_command(0x06); // Entry mode: increment cursor
	lcd16x2_write_command(disp_attr);
}
