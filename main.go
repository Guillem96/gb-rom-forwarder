package main

import (
	"flag"
	"fmt"
	"log"
	"math"
	"strconv"
	"strings"
	"time"

	"github.com/Guillem96/gameboy-tools/cartridge"
	"github.com/Guillem96/gameboy-tools/conmap"
	"github.com/Guillem96/gameboy-tools/gbproxy"
)

func main() {

	// Read CLI flags
	mpath := flag.String("mapping", "mapping.yaml", "GameBoy pins connections to RaspberryPi pins")
	romPath := flag.String("rom", "tetris.gb", "ROM to forward")
	flag.Parse()

	cr := cartridge.NewFileROMReader(*romPath)
	c, err := cr.ReadCartridge()
	if err != nil {
		log.Fatal(err)
	}

	c.Header.PrintInfo()

	// Read the RPi connections mapping to the GameBoy cartridge
	gbcon := conmap.ParseRaspberryWireMapping(*mpath)

	// Create cartridge dumper
	gbproxy := gbproxy.NewRPiGameBoyProxy(gbcon, false)
	defer gbproxy.End()

	rf := NewROMForwarder(c)

	for {
		// addr := GetValueFromPins(gbproxy.As)
		// d := GetValueFromPins(gbproxy.Db)
		// fmt.Printf("WR %v RD %v\n", gbproxy.Wr.Read(), gbproxy.Rd.Read())
		if !gbproxy.Rd.Read() && gbproxy.Wr.Read() {
			for _, d := range gbproxy.Db {
				d.Output()
			}

			addr := GetValueFromPins(gbproxy.As[:14])
			fmt.Printf("Reading... 0x%04x", addr)
			bin := strconv.FormatInt(int64(addr), 2)
			fmt.Print(" (", strings.Repeat("0", 16-len(bin)), bin, ") -> ")
			fmt.Printf("0x%2x\n", uint(rf.Read(addr)))

			writeToPins(uint(rf.Read(addr)), gbproxy.Db)
		} else if gbproxy.Rd.Read() && !gbproxy.Wr.Read() {
			fmt.Println("Writting...")
			for _, d := range gbproxy.Db {
				d.Input()
			}

			addr := GetValueFromPins(gbproxy.As)
			writebyte := GetValueFromPins(gbproxy.Db)
			rf.Write(addr, uint8(writebyte))
		}

		time.Sleep(50 * time.Microsecond)
	}
}

func GetValueFromPins(ps []gbproxy.GameBoyRPiPin) (v uint16) {
	v = 0x0
	for i, a := range ps {
		if a.Read() {
			v += (1 << i)
		}
	}
	return
}

func writeToPins(value uint, pins []gbproxy.GameBoyRPiPin) {
	for i, p := range pins {
		ab := uint(math.Pow(2, float64(i)))
		p.SetState((value & ab) >= uint(1))
	}
}
