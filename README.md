## Simple Gameboy Emulator in C

This emulator is capable of running `tetris`, and other MBC1 games with multiple ROM banks and RAM banks, like `prince of persia`.

The emulator passes all `cpu_instr` blarggs tests, and passes all MBC1 `mooneye-gb rom ram and bit` tests

It's made completely in C. Most parts are fine except for the `memory.c` which due to lack of understanding when starting out has duplicate address spaces for some parts and mimics the Nintendo console in a way that could be much simpler and much more intuitive.

## Building

* Clone the repository
* Install `glfw` with `brew install glfw` and `glew` with `brew install glew`
* `mkdir obj`
* Compile with `make`

## Running

To specify a ROM, run i.e. `./emulator -r prince-of-persia.gb`. By default `./emulator` will search for a file named `tetris-jp.gb`

## Controls

The gameboy has 8 buttons:

Control | Key
--------|-------
Left | <kbd>A</kbd>
Up | <kbd>W</kbd>
Down | <kbd>S</kbd>
Right | <kbd>D</kbd>
Button A | <kbd>J</kbd>
Button B | <kbd>K</kbd>
Start | <kbd>N</kbd>
Select | <kbd>M</kbd>

## Screenshots

![cpu_instr](https://github.com/alt-romes/gameboyemulator/blob/master/screenshots/cpu_instr.png?raw=true)

![tetris opening screen](https://github.com/alt-romes/gameboyemulator/blob/master/screenshots/tetris_1.png?raw=true)

![tetris mid game](https://github.com/alt-romes/gameboyemulator/blob/master/screenshots/tetris_2.png?raw=true)

![tetris game over](https://github.com/alt-romes/gameboyemulator/blob/master/screenshots/tetris_3.png?raw=true)

![prince of persia](https://github.com/alt-romes/gameboyemulator/blob/master/screenshots/prince_of_persia_1.png?raw=true)
