#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "roms/tetris.h"
#include "cartridge.h"
#include "fastlz.h"

#define RESET_PIN 28
#define WR_PIN 27
#define ADDR_MASK 0b1111111111111111
#define DATA_MASK 0b100011111110000000000000000

// Forwards a ROM only cartridge
void forward_rom_only(const cartridge_t *);

// Initializes a GPIO pin in output mode
void gpio_init_output(int);

// Initializes a GPIO pin in input mode
void gpio_init_input(int);

// Sets the Game Boy data bus GPIO pins in input mode
inline void gpio_data_read_mode();

// Sets the Game Boy data bus GPIO pins in output mode
inline void gpio_data_write_mode();

// Writes a byte to GPIO data bus pins
inline void gpio_write_data(uint8_t);

// Reads the state of the GPIO data bus pins
inline uint8_t gpio_read_input_data();

// Resets the Game Boy
void gameboy_reset();

int main()
{
    stdio_init_all();

    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(1000);
    set_sys_clock_khz(360000, true);

    gpio_init_output(25); // Board LED

    // Reset pin initialization
    gpio_init_output(RESET_PIN);
    gpio_put(RESET_PIN, 1);

    // Write pin initialization
    gpio_init_input(WR_PIN);

    // Initialize address pins
    gpio_init_mask(ADDR_MASK);
    gpio_set_dir_in_masked(ADDR_MASK);

    // Initialize data pins
    gpio_init_mask(DATA_MASK);
    gpio_data_write_mode();

    // Uncompress ROM
    uint8_t *uncompressed_tetris = (uint8_t *)malloc(tetris_original_size);

    // Since the ROM is compiled in Python fastlz package adds the size of the uncompressed array
    // at the front. So we skip it by adding sizeof(uint32_t) to the input pointer and substracting the
    // same amount to the input size.
    uint32_t out_size = fastlz_decompress(
        tetris + sizeof(uint32_t), tetris_compressed_size - sizeof(uint32_t),
        uncompressed_tetris, tetris_original_size);

    cartridge_t *cart = cartridge_w_custom_logo(uncompressed_tetris);
    forward_rom_only(cart);
    cartridge_free(cart);
}

void gpio_init_output(int pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

void gpio_init_input(int pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
}

void gameboy_reset()
{
    gpio_put(RESET_PIN, 0);
    sleep_ms(100);
    gpio_put(RESET_PIN, 1);
}

void gpio_data_read_mode()
{
    gpio_set_dir_in_masked(DATA_MASK);
}

void gpio_data_write_mode()
{
    gpio_set_dir_out_masked(DATA_MASK);
}

void gpio_write_data(uint8_t b)
{
    uint32_t out_data = (uint32_t)(b & 0b01111111) << 16;
    out_data |= (uint32_t)(b & 0b10000000) << 19;
    gpio_put_masked(DATA_MASK, out_data);
}

uint8_t gpio_read_input_data()
{
    uint32_t all_pins = gpio_get_all() & DATA_MASK;
    return all_pins >> 16 | all_pins >> 19;
}

void forward_rom_only(const cartridge_t *cart)
{
    uint16_t addr = 0;
    uint16_t last_addr = 0;

    set_sys_clock_khz(270000, true);
    sleep_ms(1000);
    gameboy_reset();

    while (1)
    {
        addr = gpio_get_all() & ADDR_MASK;
        if (addr != last_addr)
        {
            last_addr = addr;
            if (gpio_get(WR_PIN))
            {
                gpio_data_write_mode();
                gpio_write_data(cartridge_read(cart, addr));
            }
        }
    }
}
