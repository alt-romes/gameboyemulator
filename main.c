#include <stdio.h>
#include <stdlib.h>
#include "memory.c"
#include "cpu.c"


int main(int argc, char *argv[])
{    
    // init_memory();
    init_cpu();

    load_cartridge("tetris-jp.gb");

    for(int i=0; i<10; i++)
        execute_op();

    return 0;
}



// IDEA: make minimal version
