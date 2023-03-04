/*
 * hal_lcd.c
 *
 *  Created on: Mar 4, 2023
 *      Author: VT Labs
 */
#include "hal_lcd.h"
#include <string.h>

HAL_LCD_GPIO_t *lcd_pins;
HAL_LCD_t lcd;

#define HAL_LCD_Bit(var, bit) (var >> bit) & 0x1

static inline uint32_t HAL_LCD_DWT_Delay_Init(void) {
	/* Disable TRC */
	CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // ~0x01000000;
	/* Enable TRC */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // 0x01000000;

	/* Disable clock cycle counter */
	DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
	/* Enable  clock cycle counter */
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; //0x00000001;

	/* Reset the clock cycle counter value */
	DWT->CYCCNT = 0;

	/* 3 NO OPERATION instructions */
	__ASM volatile ("NOP");
	__ASM volatile ("NOP");
	__ASM volatile ("NOP");

	/* Check if clock cycle counter has started */
	if (DWT->CYCCNT) {
		return 0; /*clock cycle counter started*/
	} else {
		return 1; /*clock cycle counter not started*/
	}
}

static void HAL_LCD_Delay_us(volatile uint32_t microseconds) {
	uint32_t clk_cycle_start = DWT->CYCCNT;

	/* Go to number of cycles for system */
	microseconds *= (HAL_RCC_GetHCLKFreq() / 1000000);

	/* Delay till end */
	while ((DWT->CYCCNT - clk_cycle_start) < microseconds)
		;
}

static inline void HAL_LCD_PulseEnable(void) {
	HAL_GPIO_WritePin(lcd_pins->PORT_EN, lcd_pins->PIN_EN, 0);
	HAL_LCD_Delay_us(5);
	HAL_GPIO_WritePin(lcd_pins->PORT_EN, lcd_pins->PIN_EN, 1);
	HAL_LCD_Delay_us(5); // enable pulse must be >450 ns
	HAL_GPIO_WritePin(lcd_pins->PORT_EN, lcd_pins->PIN_EN, 0);
	HAL_LCD_Delay_us(100); // commands need >37 us to settle
}

static void HAL_LCD_Write4bits(uint8_t value) {
	HAL_GPIO_WritePin(lcd_pins->PORT_D4, lcd_pins->PIN_D4,
			HAL_LCD_Bit(value, 0));
	HAL_GPIO_WritePin(lcd_pins->PORT_D5, lcd_pins->PIN_D5,
			HAL_LCD_Bit(value, 1));
	HAL_GPIO_WritePin(lcd_pins->PORT_D6, lcd_pins->PIN_D6,
			HAL_LCD_Bit(value, 2));
	HAL_GPIO_WritePin(lcd_pins->PORT_D7, lcd_pins->PIN_D7,
			HAL_LCD_Bit(value, 3));
	HAL_LCD_PulseEnable();
}

static void HAL_LCD_Write8bits(uint8_t value) {
	HAL_GPIO_WritePin(lcd_pins->PORT_D0, lcd_pins->PIN_D0,
			HAL_LCD_Bit(value, 0));
	HAL_GPIO_WritePin(lcd_pins->PORT_D1, lcd_pins->PIN_D1,
			HAL_LCD_Bit(value, 1));
	HAL_GPIO_WritePin(lcd_pins->PORT_D2, lcd_pins->PIN_D2,
			HAL_LCD_Bit(value, 2));
	HAL_GPIO_WritePin(lcd_pins->PORT_D3, lcd_pins->PIN_D3,
			HAL_LCD_Bit(value, 3));
	HAL_GPIO_WritePin(lcd_pins->PORT_D4, lcd_pins->PIN_D4,
			HAL_LCD_Bit(value, 4));
	HAL_GPIO_WritePin(lcd_pins->PORT_D5, lcd_pins->PIN_D5,
			HAL_LCD_Bit(value, 5));
	HAL_GPIO_WritePin(lcd_pins->PORT_D6, lcd_pins->PIN_D6,
			HAL_LCD_Bit(value, 6));
	HAL_GPIO_WritePin(lcd_pins->PORT_D7, lcd_pins->PIN_D7,
			HAL_LCD_Bit(value, 7));
	HAL_LCD_PulseEnable();
}

static void HA_LCD_Send(uint8_t value, uint8_t mode) {
	HAL_GPIO_WritePin(lcd_pins->PORT_RS, lcd_pins->PIN_RS, mode != 0);

	if (lcd._displayfunction & LCD_COMMAND_8BITMODE) {
		HAL_LCD_Write8bits(value);
	} else {
		HAL_LCD_Write4bits(value >> 4);
		HAL_LCD_Write4bits(value);
	}
}

static void HAL_LCD_InitPin(GPIO_TypeDef *port, uint16_t pin) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(port, &GPIO_InitStruct);
}

static inline void HAL_LCD_SetPos(uint8_t col, uint8_t row) {
	const size_t max_lines = sizeof(lcd._row_offsets)
			/ sizeof(*lcd._row_offsets);
	if (row >= max_lines) {
		row = max_lines - 1;    // we count rows starting w/ 0
	}
	if (row >= lcd._numlines) {
		row = lcd._numlines - 1;    // we count rows starting w/ 0
	}

	HAL_LCD_Command(LCD_COMMAND_SETDDRAMADDR | (col + lcd._row_offsets[row]));
}

void HAL_LCD_Command(uint8_t value) {
	HA_LCD_Send(value, 0);
	HAL_LCD_Delay_us(5);
}

void HAL_LCD_Data(uint8_t value) {
	HA_LCD_Send(value, 1);
	HAL_LCD_Delay_us(5);
}

void HAL_LCD_Initialize(HAL_LCD_GPIO_t *gpio, HAL_LCD_Command_t dotsize,
		HAL_LCD_Command_t mode, uint8_t lines) {
	memset(&lcd, 0, sizeof(lcd));
	lcd_pins = gpio;
	lcd._displayfunction = mode | dotsize;

	if (lines > 1) {
		lcd._displayfunction |= LCD_COMMAND_2LINE;
	}

	lcd._numlines = lines;

	// for some 1 line displays you can select a 10 pixel high font
	if ((dotsize != LCD_COMMAND_5x8DOTS) && (lines == 1)) {
		lcd._displayfunction |= LCD_COMMAND_5x10DOTS;
	}

	// Configure pins
	if (lcd._displayfunction & LCD_COMMAND_8BITMODE) {
		HAL_LCD_InitPin(lcd_pins->PORT_D0, lcd_pins->PIN_D0);
		HAL_LCD_InitPin(lcd_pins->PORT_D1, lcd_pins->PIN_D1);
		HAL_LCD_InitPin(lcd_pins->PORT_D2, lcd_pins->PIN_D2);
		HAL_LCD_InitPin(lcd_pins->PORT_D3, lcd_pins->PIN_D3);
	}
	HAL_LCD_InitPin(lcd_pins->PORT_D4, lcd_pins->PIN_D4);
	HAL_LCD_InitPin(lcd_pins->PORT_D5, lcd_pins->PIN_D5);
	HAL_LCD_InitPin(lcd_pins->PORT_D6, lcd_pins->PIN_D6);
	HAL_LCD_InitPin(lcd_pins->PORT_D7, lcd_pins->PIN_D7);
	HAL_LCD_InitPin(lcd_pins->PORT_EN, lcd_pins->PIN_EN);
	HAL_LCD_InitPin(lcd_pins->PORT_RS, lcd_pins->PIN_RS);

	HAL_LCD_DWT_Delay_Init();

	lcd._row_offsets[0] = 0;
	lcd._row_offsets[1] = 0x40;
	lcd._row_offsets[2] = 0xF;
	lcd._row_offsets[3] = 0x4F;

	HAL_Delay(40);
	// Now we pull both RS and R/W low to begin commands
	HAL_GPIO_WritePin(lcd_pins->PORT_RS, lcd_pins->PIN_RS, 0);
	HAL_GPIO_WritePin(lcd_pins->PORT_EN, lcd_pins->PIN_EN, 0);

	// put the LCD into 4 bit or 8 bit mode
	if (!(lcd._displayfunction & LCD_COMMAND_8BITMODE)) {
		// this is according to the Hitachi HD44780 datasheet
		// figure 24, pg 46

		// we start in 8bit mode, try to set 4 bit mode
		HAL_LCD_Write4bits(0x03);
		HAL_LCD_Delay_us(4500); // wait min 4.1ms

		// second try
		HAL_LCD_Write4bits(0x03);
		HAL_LCD_Delay_us(4500); // wait min 4.1ms

		// third go!
		HAL_LCD_Write4bits(0x03);
		HAL_LCD_Delay_us(150);

		// finally, set to 4-bit interface
		HAL_LCD_Write4bits(0x02);
	} else {
		// this is according to the Hitachi HD44780 datasheet
		// page 45 figure 23

		// Send function set HAL_LCD_Command sequence
		HAL_LCD_Command(LCD_COMMAND_FUNCTIONSET | lcd._displayfunction);
		HAL_LCD_Delay_us(4500); // wait more than 4.1 ms

		// second try
		HAL_LCD_Command(LCD_COMMAND_FUNCTIONSET | lcd._displayfunction);
		HAL_LCD_Delay_us(150);

		// third go
		HAL_LCD_Command(LCD_COMMAND_FUNCTIONSET | lcd._displayfunction);
	}

	// finally, set # lines, font size, etc.
	HAL_LCD_Command(LCD_COMMAND_FUNCTIONSET | lcd._displayfunction);

	// turn the display on with no cursor or blinking default
	lcd._displaycontrol = LCD_COMMAND_DISPLAYON | LCD_COMMAND_CURSOROFF
			| LCD_COMMAND_BLINKOFF;
	HAL_LCD_Display();

	// clear it off
	HAL_LCD_Clear();

	// Initialize to default text direction (for romance languages)
	lcd._displaymode = LCD_COMMAND_ENTRYLEFT | LCD_COMMAND_ENTRYSHIFTDECREMENT;
	// set the entry mode
	HAL_LCD_Command(LCD_COMMAND_ENTRYMODESET | lcd._displaymode);
}

/********** high level commands, for the user! */
void HAL_LCD_Clear() {
	HAL_LCD_Command(LCD_COMMAND_CLEARDISPLAY); // clear display, set cursor position to zero
	HAL_LCD_Delay_us(2000);           // this HAL_LCD_Command takes a long time!
}

void HAL_LCD_Home() {
	HAL_LCD_Command(LCD_COMMAND_RETURNHOME); // set cursor position to zero
	HAL_LCD_Delay_us(2000);          // this HAL_LCD_Command takes a long time!
}

// Turn the display on/off (quickly)
void HAL_LCD_NoDisplay() {
	lcd._displaycontrol &= ~LCD_COMMAND_DISPLAYON;
	HAL_LCD_Command(LCD_COMMAND_DISPLAYCONTROL | lcd._displaycontrol);
}
void HAL_LCD_Display() {
	lcd._displaycontrol |= LCD_COMMAND_DISPLAYON;
	HAL_LCD_Command(LCD_COMMAND_DISPLAYCONTROL | lcd._displaycontrol);
}

void HAL_LCD_Mode(uint8_t Cursor, uint8_t Blinking) {
	uint8_t Cmd;
	Cmd = LCD_COMMAND_DISPLAYON
			| (Cursor ? LCD_COMMAND_CURSORON : LCD_COMMAND_CURSOROFF)
			| (Blinking ? LCD_COMMAND_BLINKON : LCD_COMMAND_BLINKOFF);
	HAL_LCD_Command(Cmd | LCD_COMMAND_DISPLAYCONTROL); // display clear
} // LCD_Mode

void HA_LCD_Delete(uint8_t col, uint8_t row, uint8_t len) {
	HAL_LCD_Position(col, row);
	while (len--) {
		HAL_LCD_Char(' ');
	}
}

void HAL_LCD_Char(char chr) {
	lcd.lines_updated[lcd.y][lcd.x] = chr;
	++lcd.x;
	if (lcd.x > 15) {
		lcd.x = 0;
	}
}

void HAL_LCD_String(char *str) {
	while (*str) {
		HAL_LCD_Char(*str++);
	}
}

void HAL_LCD_ConstString(const char *str) {
	char buf[17] = { 0 };
	strcpy(buf, str);
	HAL_LCD_String(buf);
}

void HAL_LCD_Position(uint8_t x, uint8_t y) {
	if (x < 16 && y < 4) {
		lcd.x = x;
		lcd.y = y;
	}
}

void HAL_LCD_Update(void) {
	for (uint8_t l = 0; l < 4; l++) {
		for (uint8_t c = 0; c < 16; c++) {
			if (lcd.lines[l][c] != lcd.lines_updated[l][c]) {
				lcd.lines[l][c] = lcd.lines_updated[l][c];
				HAL_LCD_SetPos(c, l);
				HAL_LCD_Data(lcd.lines[l][c]);
			}
		}
	}
}

