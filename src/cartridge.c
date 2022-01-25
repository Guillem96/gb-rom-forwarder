#include "cartridge.h"

#include <stdlib.h>
#include "pico/stdlib.h"

const uint8_t custom_logo[] = {
    0x36, 0xcc, 0xc6, 0x00, 0x00, 0x0c, 0x11, 0x0d, 0x8b, 0x3b, 0x06, 0x66, 0x00, 0x07, 0x00, 0x09,
    0x00, 0x0a, 0x00, 0x04, 0xf9, 0x9f, 0xf8, 0x88, 0xcc, 0x73, 0xe6, 0xec, 0xcc, 0xed, 0xdd, 0xd9,
    0xbb, 0xbb, 0x66, 0x66, 0xcf, 0xc7, 0xdd, 0x1d, 0xfb, 0xbb, 0xe6, 0x66, 0x11, 0x9f, 0xf9, 0x9f,
};

static uint8_t read_w_custom_logo(cartridge_t *, uint16_t);
static inline uint8_t read_rom(const cartridge_t *, uint16_t);

cartridge_t *cartridge_w_custom_logo(const uint8_t *rom)
{
    cartridge_t *cart = (cartridge_t *)malloc(sizeof(cartridge_t));
    cart->bytes = (uint8_t *)rom;
    cart->_read = (uint8_t(*)(const cartridge_t *, uint16_t))read_w_custom_logo;
    return cart;
}

cartridge_t *cartridge_wo_custom_logo(const uint8_t *rom)
{
    cartridge_t *cart = (cartridge_t *)malloc(sizeof(cartridge_t));
    cart->bytes = (uint8_t *)rom;
    cart->_read = read_rom;
    return cart;
}

uint8_t cartridge_read(const cartridge_t *cart, uint16_t address)
{
    return cart->_read(cart, address);
}

uint8_t read_w_custom_logo(cartridge_t *cart, uint16_t address)
{
    static uint8_t reads = 0;
    uint8_t data;

    if (address >= 0x104 && address <= 0x133 && (reads == 1 || reads == 2))
        data = custom_logo[address - 0x104];
    else
        data = cart->bytes[address];

    if (address == 0x133)
    {
        reads++;
        if (reads == 3)
        {
            // Logo has already been read, reference to the "simple" read function.
            cart->_read = read_rom;
        }
    }
    return data;
}

uint8_t read_rom(const cartridge_t *cart, uint16_t address)
{
    return cart->bytes[address];
}


void cartridge_free(cartridge_t *cart)
{
    free(cart);
}