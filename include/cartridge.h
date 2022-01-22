#ifndef _CARTRIDGE_H_
#define _CARTRIDGE_H_

#include <stdint.h>

typedef struct
{
    uint8_t* bytes;
} cartridge_t;


uint8_t read(const cartridge_t* cart, uint16_t address);

#endif
