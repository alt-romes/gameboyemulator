#include <stdio.h>
#include <stdlib.h>
#include "memory.c"
#include "cpu.c"
#include "ppu.c"


void emulate() {

    debug();
    int i=0;
    while (registers.pc < 257 && i++ < 4263) {
        printf("Instruction Number: %d\n", i);
        cpu();
    }

}


int main(int argc, char *argv[])
{    
    load_bootstrap_rom();

    boot();

    load_cartridge("tetris-jp.gb");

    emulate();

    print_tiles();

    return 0;
}



// IDEA: make smallest possible version
