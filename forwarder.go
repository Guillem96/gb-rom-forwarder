package main

import (
	"fmt"
	"os"
	"time"

	"github.com/Guillem96/gameboy-tools/cartridge"
)

const (
	RamEnableKey      = "ramenable"
	RomBankNumberKey  = "rombanknumber"
	HighRomBankBitKey = "highrombanknumber"
	RamBankNumberKey  = "rambanknumber"
	RTCNumberKey      = "rtcnumber"
	IsRTCSelectedKey  = "rtcselected"
	LatchClockDataKey = "latchclockdata"
	BankingModeKey    = "bankmode"
)

const (
	RTC_S  = 0x08
	RTC_M  = 0x09
	RTC_H  = 0x0A
	RTC_DL = 0x0B
	RTC_DH = 0x0C
)

type ROMRegisters map[string]uint8

func (rr ROMRegisters) Get(key string, def uint8) uint8 {
	v, f := rr[key]
	if !f {
		return def
	}
	return v
}

type ROMForwarder struct {
	Cart           *cartridge.Cartridge
	CurrentRomBank int
	CurrentRamBank int
	regs           ROMRegisters

	// For MBC3
	rtc map[uint8]uint8
}

func NewROMForwarder(forward *cartridge.Cartridge) *ROMForwarder {
	return &ROMForwarder{
		Cart:           forward,
		CurrentRomBank: 1,
		CurrentRamBank: 0,
		regs:           make(ROMRegisters),
		rtc:            make(map[uint8]uint8),
	}
}

func (rf *ROMForwarder) Read(addr uint16) uint8 {
	if addr > 0x4000 {
		return rf.Cart.ROMBanks[rf.CurrentRomBank][addr-0x4000]
	} else if addr < 0x8000 {
		// MBC1 oddity where bank 0 can be overwritten in advanced banking mode
		if rf.Cart.Header.IsMBC1() &&
			rf.regs.Get(BankingModeKey, 0x0) == 0x1 &&
			rf.Cart.Header.ROMSize >= cartridge.ROM1MB &&
			rf.regs.Get(RomBankNumberKey, 0x0) == 0x0 {
			return rf.Cart.ROMBanks[int(rf.regs.Get(RomBankNumberKey, 0x0)<<5)][addr]
		} else {
			return rf.Cart.ROMBanks[0][addr]
		}
	} else if rf.Cart.Header.HasRAM() {
		if rf.Cart.Header.IsMBC3() && addr >= 0xA000 && addr <= 0xC000 {
			if rf.regs.Get(IsRTCSelectedKey, 0x0) == 0x2 {
				// Read latched clock data
				return rf.rtc[rf.regs[RTCNumberKey]]
			} else if rf.regs.Get(IsRTCSelectedKey, 0x0) == 0x1 {
				// TODO: Read ram bank
				return 0xFF
			}
		}

		// TODO: Read ram
		return 0xFF
	}
	return 0xFF
}

func (rf *ROMForwarder) ReadRam(addr uint16) uint8 {
	if !rf.Cart.Header.HasRAM() {
		return 0xFF
	} else if !rf.IsRamEnabled() {
		return 0xFF
	} else {
		// TODO: Parse rombanks as well
		return 0x0
	}
}

func (rf *ROMForwarder) IsRamEnabled() bool {
	if !rf.Cart.Header.HasRAM() {
		return false
	}

	if (rf.regs.Get(RamEnableKey, 0x0) & 0xF) == 0xA {
		return true
	}

	return false
}

func (rf *ROMForwarder) Write(addr uint16, value uint8) {
	if !rf.Cart.Header.HasMBC() {
		return
	} else if rf.Cart.Header.IsMBC1() {
		rf.mBC1HandleWrite(addr, value)
	} else if rf.Cart.Header.IsMBC2() {
		rf.mBC2HandleWrite(addr, value)
	} else if rf.Cart.Header.IsMBC3() {
		rf.mBC3HandleWrite(addr, value)
	} else if rf.Cart.Header.IsMBC5() {
		rf.mBC5HandleWrite(addr, value)
	} else {
		fmt.Println("Cartridge type not supported")
		os.Exit(1)
	}
}

func (rf *ROMForwarder) mBC1HandleWrite(addr uint16, value uint8) {
	if addr >= 0x0 && addr < 0x2000 {
		// RAM Enable
		rf.regs[RamEnableKey] = value & 0xF
		rf.CurrentRamBank = rf.computeRamBank()
	} else if addr >= 0x2000 && addr < 0x4000 {
		// ROM Bank Number
		rf.regs[RomBankNumberKey] = value & 0x1F
		rf.CurrentRomBank = rf.computeRomBank()
	} else if addr >= 0x4000 && addr < 0x6000 {
		// RAM Bank Number - or - Upper Bits of ROM Bank Number
		rf.regs[RamBankNumberKey] = value & 0x3
		rf.CurrentRomBank = rf.computeRomBank()
		rf.CurrentRamBank = rf.computeRamBank()
	} else if addr >= 0x6000 && addr < 0x8000 {
		rf.regs[BankingModeKey] = value & 0x1
		if rf.IsRamEnabled() {
			rf.CurrentRamBank = rf.computeRamBank()
		}
	}
}

func (rf *ROMForwarder) mBC2HandleWrite(addr uint16, value uint8) {
	if addr >= 0x0 && addr < 0x4000 {
		cb := (addr >> 8) & 0x1
		if cb == 1 {
			rf.regs[RomBankNumberKey] = value & 0xF
			rf.CurrentRomBank = rf.computeRomBank()
		} else {
			// RAM Enable
			rf.regs[RamEnableKey] = value & 0xF
			rf.CurrentRamBank = rf.computeRamBank()
		}
	}
}

func (rf *ROMForwarder) mBC3HandleWrite(addr uint16, value uint8) {
	if addr >= 0x0 && addr < 0x2000 {
		// RAM Enable
		rf.regs[RamEnableKey] = value & 0xF
		rf.CurrentRamBank = rf.computeRamBank()
	} else if addr >= 0x2000 && addr < 0x4000 {
		// ROM Bank Number
		rf.regs[RomBankNumberKey] = value & 0x1F
		rf.CurrentRomBank = rf.computeRomBank()
	} else if addr >= 0x4000 && addr < 0x6000 {
		rf.regs[IsRTCSelectedKey] = 0x0
		if value >= 0x0 && value < 0x4 {
			rf.regs[RamBankNumberKey] = value
			rf.regs[IsRTCSelectedKey] = 0x1
			rf.CurrentRamBank = rf.computeRamBank()
		} else if value >= 0x8 && value < 0xD {
			rf.regs[RTCNumberKey] = value
			rf.regs[IsRTCSelectedKey] = 0x2
		}
	} else if addr >= 0x6000 && addr < 0x8000 {
		v, f := rf.regs[LatchClockDataKey]
		if f && v == 0x0 && value == 0x1 {
			cd := time.Now()
			rf.rtc[RTC_S] = uint8(cd.Second())
			rf.rtc[RTC_M] = uint8(cd.Minute())
			rf.rtc[RTC_H] = uint8(cd.Hour())
			rf.rtc[RTC_DL] = uint8(cd.Day() & 0xFF)
			rf.rtc[RTC_DH] = uint8((cd.Day() >> 8) & 0x1)
		}
		rf.regs[LatchClockDataKey] = value
	}
}

func (rf *ROMForwarder) mBC5HandleWrite(addr uint16, value uint8) {
	if addr >= 0x0 && addr < 0x2000 {
		// RAM Enable
		rf.regs[RamEnableKey] = value & 0xF
		rf.CurrentRamBank = rf.computeRamBank()
	} else if addr >= 0x2000 && addr < 0x3000 {
		// ROM Bank Number
		rf.regs[RomBankNumberKey] = value
		rf.CurrentRomBank = rf.computeRomBank()
	} else if addr >= 0x3000 && addr < 0x4000 {
		// ROM Bank Number
		rf.regs[HighRomBankBitKey] = value & 0x1
		rf.CurrentRomBank = rf.computeRomBank()
	} else if addr >= 0x4000 && addr < 0x6000 {
		rf.regs[RamBankNumberKey] = value
		rf.CurrentRamBank = rf.computeRamBank()
	}
}

func (rf *ROMForwarder) computeRomBank() int {
	if rf.Cart.Header.IsMBC1() {
		lb := rf.regs.Get(RomBankNumberKey, 0x0)
		hb := rf.regs.Get(RamBankNumberKey, 0x0)

		if lb == 0x0 { // Cannot index 0x0, 0x20, 0x40, 0x60
			// Those banks are indexed in the 0x0 - 0x3FFF address range, see Read function.
			return int((hb << 5) + 1)
		}

		return int((hb << 5) | lb)
	} else if rf.Cart.Header.IsMBC2() || rf.Cart.Header.IsMBC3() {
		return int(rf.regs.Get(RomBankNumberKey, 0x0))
	} else if rf.Cart.Header.IsMBC3() {
		bank := rf.regs.Get(RomBankNumberKey, 0x0)
		if bank == 0x0 {
			return 1
		}
		return int(bank)
	} else if rf.Cart.Header.IsMBC5() {
		lb := uint16(rf.regs.Get(RomBankNumberKey, 0x0))
		hb := uint16(rf.regs.Get(RomBankNumberKey, 0x0))
		return int(hb<<8 | lb)
	}
	return 0
}

func (rf *ROMForwarder) computeRamBank() int {
	if rf.Cart.Header.IsMBC1() {
		if rf.regs.Get(BankingModeKey, 0x0) == 0x0 {
			return 0
		} else if rf.Cart.Header.RAMSize > cartridge.RAM8KB {
			return int(rf.regs.Get(RamBankNumberKey, 0x0))
		}
	}

	// MBC2 only has ram bank 0

	if rf.Cart.Header.IsMBC3() {
		if rf.regs.Get(IsRTCSelectedKey, 0x0) == 0x1 {
			return int(rf.regs.Get(RamBankNumberKey, 0x0))
		} else {
			return 0
		}
	}

	if rf.Cart.Header.IsMBC5() {
		return int(rf.regs.Get(RamBankNumberKey, 0x0))
	}

	return 0
}
