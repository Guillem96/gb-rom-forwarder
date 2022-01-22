#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "roms/tetris.h"

#define RESET_PIN 28
#define WR_PIN 27
#define ADDR_MASK 0b1111111111111111
#define DATA_MASK 0b100011111110000000000000000

void forward_rom_only();
void gpio_init_output(int);
void gpio_init_input(int);
inline void gpio_data_read_mode();
inline void gpio_data_write_mode();
inline uint8_t gpio_read_input_data();
void gameboy_reset();

int main()
{
    stdio_init_all();

    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(1000);
    set_sys_clock_khz(360000, true);

    gpio_init_output(25);

    // Reset pin inizialization
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

    forward_rom_only();
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

uint8_t gpio_read_input_data()
{
    uint32_t allPins = gpio_get_all() & DATA_MASK;
    return allPins >> 16 | allPins >> 19;
}

void forward_rom_only()
{
    set_sys_clock_khz(270000, true);
    sleep_ms(1000);
    gameboy_reset();

    static uint16_t lastAddress = 0;
    uint16_t address = 0;
    uint32_t outputData = 0;

    while (true)
    {
        address = gpio_get_all() & ADDR_MASK;
        if (address != lastAddress)
        {
            lastAddress = address;
            if (gpio_get(WR_PIN))
            {
                gpio_data_write_mode();

                outputData = 0;
                outputData |= ((uint32_t)(tetris[address]) & 0b01111111) << 16;
                outputData |= ((uint32_t)(tetris[address] >> 7)) << 26;

                gpio_put_masked(DATA_MASK, outputData);
            }
        }
    }
}
