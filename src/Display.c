#include "Display.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"

// ---------------------------------------------------------------------------
// Hardware SPI pins
// ---------------------------------------------------------------------------

#define SPI_PORT spi0
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// ---------------------------------------------------------------------------
// MAX7219 register addresses
// ---------------------------------------------------------------------------

#define REG_DECODE_MODE  0x09
#define REG_INTENSITY    0x0A
#define REG_SCAN_LIMIT   0x0B
#define REG_SHUTDOWN     0x0C
#define REG_DISPLAY_TEST 0x0F

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Send one 16-bit command word (register + data) to the MAX7219.
static void max7219_write(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, buf, 2);
    gpio_put(PIN_CS, 1);
}

static void max7219_clear(void) {
    for (int row = 1; row <= 8; row++) {
        max7219_write(row, 0x00);
    }
}

static void max7219_init(void) {
    max7219_write(REG_DISPLAY_TEST, 0x00); // Display-test off
    max7219_write(REG_SCAN_LIMIT,   0x07); // Enable all 8 rows
    max7219_write(REG_DECODE_MODE,  0x00); // Raw (matrix) mode – no BCD decode
    max7219_write(REG_INTENSITY,    0x01); // Low brightness
    max7219_write(REG_SHUTDOWN,     0x01); // Normal operation
    max7219_clear();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void init_display(void) {
    // SPI at 1 MHz
    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Chip-select (active low, idle high)
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    max7219_init();
}

void display_pattern(uint8_t pattern[8]) {
    // MAX7219 row registers are 1-indexed (row 1 = register 0x01)
    for (int i = 0; i < 8; i++) {
        max7219_write(i + 1, pattern[i]);
    }
}