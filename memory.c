/*
 *  Gameboy Emulator: Memory
 */

// memory size 2^16 = 0xFFFF
unsigned char memory[0xFFFF] = {0};

void load_cartridge(char* filename) {

    FILE* rom = fopen(filename, "r");

    int opcode;
    for(int i=0; (opcode = getc(rom)) != EOF && i<0xFFFF; i++)
        memory[i] = (unsigned char) opcode;
        
}
