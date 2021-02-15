## Simple Gameboy Emulator in C

This emulator is capable of running `tetris`, and other MBC1 games with multiple ROM banks but without RAM banks, like `prince of persia`.

The emulator passes all `cpu_instr` blarggs tests, and passes all MBC `rom_512kb` through `rom_16Mb` - currently it does not pass `ram_64kb` and i cannot fix this bug.


## Building

* Clone the repository
* Install `glfw` with `brew install glfw` and `glew` with `brew install glew`
* Compile with `make`

## Running

To specify a ROM, run i.e. `./emulator -r prince-of-persia.gb`. By default `./emulator` will search for a file named `tetris-jp.gb`
