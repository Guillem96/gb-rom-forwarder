#ifndef _CARTRIDGE_H_
#define _CARTRIDGE_H_

#include <stdint.h>

#define ROM_ONLY 0x00
#define MBC1 0x01
#define MBC1RAM 0x02
#define MBC1RAM_BATTERY 0x03
#define MBC2 0x05
#define MBC2_BATTERY 0x06
#define ROMRAM 0x08
#define ROMRAM_BATTERY 0x09
#define MMM01 0x0B
#define MMM01RAM 0x0C
#define MMM01RAM_BATTERY 0x0D
#define MBC3_TIMER_BATTERY 0x0F
#define MBC3_TIMERRAM_BATTERY 0x10
#define MBC3 0x11
#define MBC3RAM 0x12
#define MBC3RAM_BATTERY 0x13
#define MBC5 0x19
#define MBC5RAM 0x1A
#define MBC5RAM_BATTERY 0x1B
#define MBC5_RUMBLE 0x1C
#define MBC5_RUMBLERAM 0x1D
#define MBC5_RUMBLERAM_BATTERY 0x1E
#define MBC6 0x20
#define MBC7_SENSOR_RUMBLERAM_BATTERY 0x22
#define POCKET_CAMERA 0xFC
#define BANDAITAMA5 0xFD
#define HUC3 0xFE
#define HUC1RAM_BATTERY 0xFF

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

typedef struct T
{
    uint8_t *bytes;

    // Pointer to the read function that will be executed on every Game Boy read.
    // This is for performance reasons: In case user wants to have a custom logo,
    // the read function has to always do an if branch to intercept the cartridge logo reads
    // and return the custom logo accordingly. Obviously this has an overhead of every time cheking if the
    // read is within the logo address or not. For this reason, once the custom logo has been read
    // and displayed in the Game Boy screen, we replace this pointer with a simple read function that
    // does not perform any check regarding the logo read and just forwards the corresponding rom byte.
    // If the cartridge struct is created with the cartridge_wo_custom_logo the _read pointer directly
    // referece this "simple" read function.
    uint8_t (*_read)(const struct T *, uint16_t);
} cartridge_t;

// Creates a cartridge_t struct that forwards a custom logo when Game Boy boots
cartridge_t *cartridge_w_custom_logo(const uint8_t *rom);

// Creates a cartridge_t struct that forwards the expected Nintendo logo on boot
cartridge_t *cartridge_wo_custom_logo(const uint8_t *rom);

// Reads a byte from the in memory cartridge
uint8_t cartridge_read(const cartridge_t *cart, uint16_t address);

// Removes allocated memory
void cartridge_free(cartridge_t *cart);

#endif
