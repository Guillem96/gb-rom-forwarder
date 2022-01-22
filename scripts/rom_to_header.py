#!/usr/bin/env python3

from pathlib import Path
import argparse


def main():
    args = _parse_args()

    input_path = Path(args.input)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    name = args.name if args.name else input_path.stem

    rom_bytes = input_path.read_bytes()
    rom_n_bytes = input_path.stat().st_size
    with output_path.open("w") as header_f:
        input_path.__sizeof__()
        print(f"const unsigned int {name}_size = {rom_n_bytes};\n",
              file=header_f)
        print(f"unsigned char {name}[] = {{\n    ", end="", file=header_f)

        for i, bytes_chunk in enumerate(_chunker(rom_bytes, 16)):
            print(", ".join(f"0x{b:02x}" for b in bytes_chunk),
                  end=",\n",
                  file=header_f)

            if i < len(rom_bytes) // 16 - 1:
                print("    ", end="", file=header_f)

        print("};", file=header_f)


def _chunker(it, size=16):
    return (it[i:i + size] for i in range(0, len(it), size))


def _parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Convert Game Boy roms to header files")

    parser.add_argument("input", help="Game Boy rom. Usually a .gb")
    parser.add_argument("output", help="Generated header file path.")
    parser.add_argument("--name",
                        default=None,
                        help="Name of the variable containing the rom bytes "
                        "array. If not set the name is taken from the "
                        "filename (e.g. roms/tetris.gb -> tetris).")

    return parser.parse_args(argv)


if __name__ == "__main__":
    main()
    exit(0)
