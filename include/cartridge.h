#ifndef _CARTRIDGE_H_
#define _CARTRIDGE_H_

#include <stdint.h>

const uint8_t custom_logo[];

typedef struct __attribute__((__packed__))
{
    uint8_t entrypoint[0x4];
    uint8_t nintendo_logo[0x30]; 
    uint8_t title[0x10];
    uint8_t licensee_code[0x2];
    uint8_t sgb_flag;
    uint8_t cart_type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t destination_code;
    uint8_t old_licensee_code;
    uint8_t mask_rom_version;
    uint8_t header_checksum;
    uint8_t global_checksum[0x2];
} cartridge_header_t;

typedef struct
{
    uint8_t* bytes;
} cartridge_t;


uint8_t cartridge_read(const cartridge_t* cart, uint16_t address);

#endif
