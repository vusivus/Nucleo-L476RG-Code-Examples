/*
 * hal_lcd.h
 *
 *  Created on: Mar 4, 2023
 *      Author: VT Labs
 */

#ifndef HAL_LCD_H_
#define HAL_LCD_H_
#include "main.h"

#define LCD_LINE1 0
#define LCD_LINE2 1

typedef enum {
	LCD_COMMAND_CLEARDISPLAY = 0x01,
	LCD_COMMAND_RETURNHOME = 0x02,
	LCD_COMMAND_ENTRYMODESET = 0x04,
	LCD_COMMAND_DISPLAYCONTROL = 0x08,
	LCD_COMMAND_CURSORSHIFT = 0x10,
	LCD_COMMAND_FUNCTIONSET = 0x20,
	LCD_COMMAND_SETCGRAMADDR = 0x40,
	LCD_COMMAND_SETDDRAMADDR = 0x80,
	// flags for display entry _display_mode
	LCD_COMMAND_ENTRYRIGHT = 0x00,
	LCD_COMMAND_ENTRYLEFT = 0x02,
	LCD_COMMAND_ENTRYSHIFTINCREMENT = 0x01,
	LCD_COMMAND_ENTRYSHIFTDECREMENT = 0x00,
	// flags for display on/off control
	LCD_COMMAND_DISPLAYON = 0x04,
	LCD_COMMAND_DISPLAYOFF = 0x00,
	LCD_COMMAND_CURSORON = 0x02,
	LCD_COMMAND_CURSOROFF = 0x00,
	LCD_COMMAND_BLINKON = 0x01,
	LCD_COMMAND_BLINKOFF = 0x00,
	// flags for display/cursor shift
	LCD_COMMAND_DISPLAYMOVE = 0x08,
	LCD_COMMAND_CURSORMOVE = 0x00,
	LCD_COMMAND_MOVERIGHT = 0x04,
	LCD_COMMAND_MOVELEFT = 0x00,
	// flags for function set
	LCD_COMMAND_8BITMODE = 0x10,
	LCD_COMMAND_4BITMODE = 0x00,
	LCD_COMMAND_2LINE = 0x08,
	LCD_COMMAND_1LINE = 0x00,
	LCD_COMMAND_5x10DOTS = 0x04,
	LCD_COMMAND_5x8DOTS = 0x00
} HAL_LCD_Command_t;

typedef struct {
	GPIO_TypeDef *PORT_D0;
	GPIO_TypeDef *PORT_D1;
	GPIO_TypeDef *PORT_D2;
	GPIO_TypeDef *PORT_D3;
	GPIO_TypeDef *PORT_D4;
	GPIO_TypeDef *PORT_D5;
	GPIO_TypeDef *PORT_D6;
	GPIO_TypeDef *PORT_D7;
	GPIO_TypeDef *PORT_EN;
	GPIO_TypeDef *PORT_RS;
	uint16_t PIN_D0;
	uint16_t PIN_D1;
	uint16_t PIN_D2;
	uint16_t PIN_D3;
	uint16_t PIN_D4;
	uint16_t PIN_D5;
	uint16_t PIN_D6;
	uint16_t PIN_D7;
	uint16_t PIN_EN;
	uint16_t PIN_RS;
} HAL_LCD_GPIO_t;

typedef struct {
	uint8_t _displaymode;
	uint8_t _displayfunction;
	uint8_t _displaycontrol;
	uint8_t _initialized;
	uint8_t _numlines;
	uint8_t _row_offsets[4];
	char lines[4][16];
	char lines_updated[4][16];
	uint8_t x;
	uint8_t y;
} HAL_LCD_t;

void HAL_LCD_Initialize(HAL_LCD_GPIO_t *gpio, HAL_LCD_Command_t dotsize,
		HAL_LCD_Command_t mode, uint8_t lines);
void HAL_LCD_Data(uint8_t value);
void HAL_LCD_Command(uint8_t value);
void HAL_LCD_Clear(void);
void HAL_LCD_Home(void);
void HAL_LCD_NoDisplay(void);
void HAL_LCD_Display(void);
void HA_LCD_Delete(uint8_t col, uint8_t row, uint8_t len);
void HAL_LCD_Char(char chr);
void HAL_LCD_String(char *str);
void HAL_LCD_ConstString(const char *str);
void HAL_LCD_Position(uint8_t x, uint8_t y);
void HAL_LCD_Update(void);
void HAL_LCD_Mode(uint8_t Cursor, uint8_t Blinking);

#endif /* INC_HAL_LCD_H_ */
