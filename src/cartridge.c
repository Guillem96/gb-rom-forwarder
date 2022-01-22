#include "cartridge.h"

uint8_t read(const cartridge_t* cart, uint16_t address)
{
    return cart->bytes[address];
}