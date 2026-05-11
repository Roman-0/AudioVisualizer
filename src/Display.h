#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

// Initialize the SPI bus and MAX7219 LED matrix controller.
void init_display(void);

// Write an 8-byte row pattern directly to the MAX7219.
// pattern[0] maps to row 1, pattern[7] to row 8.
void display_pattern(uint8_t pattern[8]);

#endif // DISPLAY_H