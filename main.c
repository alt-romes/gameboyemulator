#include <stdlib.h>
#include "cpu.c"

int main(int argc, char *argv[])
{    

    FILE* rom = fopen("tetris-jp.gb", "rb");

    int i = 0;
    int opcode;
    while( (opcode = getc(rom)) != EOF && i<10) {
        execute_op(opcode, rom);
        i++;
    }

    return 0;
}
