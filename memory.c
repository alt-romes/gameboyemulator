/*
 *  Gameboy Emulator: Memory
 */

// memory size 2^16 = 0xFFFF
unsigned char memory[0xFFFF+1] = {0};

void load_bootstrap_rom() {

    FILE* bootstrap = fopen("bootstrap_rom", "rb");
    fread(memory, sizeof(unsigned char), 256, bootstrap);

}

void load_cartridge(char* filename) {

    FILE* cartridge = fopen(filename, "rb");
    fread(memory, sizeof(unsigned char), 32768, cartridge);
}

