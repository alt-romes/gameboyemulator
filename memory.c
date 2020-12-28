/*
 *  Gameboy Emulator: Memory
 */

// memory size 2^16 = 0xFFFF
unsigned char memory[0xFFFF+1] = {0};

void load_bootstrap_rom() {

    FILE* bootstrap = fopen("bootstrap_rom", "r");

    int opcode;
    for(int i=0; (opcode = getc(bootstrap)) != EOF && i<0xFFFF+1; i++)
        memory[i] = (unsigned char) opcode;

}

void load_cartridge(char* filename) {

    FILE* cartridge = fopen(filename, "r");

    int opcode;
    for(int i=0; (opcode = getc(cartridge)) != EOF && i<0xFFFF+1; i++)
        memory[i] = (unsigned char) opcode;
        
}

