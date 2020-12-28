#include <stdio.h>
#include <stdlib.h>
#include "memory.c"
#include "cpu.c"


int main(int argc, char *argv[])
{    
    /* load_bootstrap_rom(); */
    load_cartridge("tetris-jp.gb");
    /* load_cartridge("tetris-win.gb"); */
    boot();
    return 0;
}



// IDEA: make smallest possible version
