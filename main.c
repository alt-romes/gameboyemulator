#include <stdio.h>
#include <stdlib.h>
#include "memory.c"
#include "cpu.c"


int main(int argc, char *argv[])
{    
    load_bootstrap_rom();
    boot();

    /* load_cartridge("tetris-jp.gb"); */

    return 0;
}



// IDEA: make smallest possible version
