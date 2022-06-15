#pragma once
#include <inttypes.h>
#include "font6x8AJ.h"


#define F_CPU 8000000UL
#include <util/delay.h>

void DELAY_MS(int ms);

// Defines for OLED output
#define OLED_SCL   PORTB2
#define OLED_SDA   PORTB0
#define OLED_SA    0x78  // Slave address


#define DIGITAL_WRITE_HIGH(PORT) PORTB |= (1 << PORT)
#define DIGITAL_WRITE_LOW(PORT) PORTB &= ~(1 << PORT)

#define SET_MODE_OUTPUT(PORT) DDRB |= (1 << PORT)
#define SET_MODE_INPUT(PORT) DDRB &= ~(1 << PORT)


// Drawing functions - adapted from those at https://github.com/andyhighnumber/Attiny-Arduino-Games
// who adapted from those at https://bitbucket.org/tinusaur/ssd1306xled
void oled_init(void);
void oled_xfer_start(void);
void oled_xfer_stop(void);
void oled_send_byte(uint8_t byte);
void oled_send_command(uint8_t command);
void oled_send_data_start(void);
void oled_send_data_stop(void);
void oled_setpos(uint8_t x, uint8_t y);
void oled_fillscreen(uint8_t fill_Data);
void oled_char_f6x8(uint8_t x, uint8_t y, const char ch[]);

