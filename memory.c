/*
 *  Gameboy Emulator: Memory
 *
 *
 */

// Gameboy random access memory
unsigned char memory[0x10000] = {0};

// Gameboy game read only memory (inserted cartridge)
unsigned char rom[0x200000] = {0};

// Gameboy bootstrap read only memory (hardcoded boot)
unsigned char bootrom[256] = {0};


void load_bootstrap_rom() {

    FILE* bootstrap = fopen("bootstrap_rom", "rb");
    fread(memory, sizeof(unsigned char), 256, bootstrap);

    fclose(bootstrap);

}

void load_cartridge(char* filename) {

    FILE* cartridge = fopen(filename, "rb");
    fread(memory+256, sizeof(unsigned char), 32768, cartridge);

    fclose(cartridge);
}

