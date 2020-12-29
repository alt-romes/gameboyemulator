/*
 *  Gameboy Emulator: Memory
 *
 *  Resources:
 *
 *  > Memory map
 *  http://www.codeslinger.co.uk/pages/projects/gameboy/files/GB.pdf
 *
 *  > Memory map
 *  http://bgb.bircd.org/pandocs.htm#memorymap
 *
 */

// Gameboy address space (RAM + VRAM?)
union address_space {
    struct {
        unsigned char rombank_fixed[0x4000]; // 16k for ROM Bank 0
        unsigned char rombank_switchable[0x4000]; // 16k for Switchable ROM Bank (1..N)
        unsigned char vram[0x2000]; // 8K for VRAM
        unsigned char external_ram[0x2000]; // 8K for External Switchable RAM in Cartridge
        unsigned char ram[0x2000]; // 8K for Internal Work RAM
        unsigned char echo[0x1E00]; // Echo of Internal RAM
        unsigned char oam[0xA0]; // Object Attribute Memory (Sprite Attribute Table)
        unsigned char unusable[0x60]; // Not usable
        union {
            struct {
                unsigned char padding[0xF];
                unsigned char interrupt_request_register[1];
                unsigned char padding_after[0x70];
            };
            unsigned char ioports[0x80]; // IO Ports
        };
        unsigned char hram[0x7F]; // High RAM
        unsigned char interrupt_enable_register[1];
    };
    unsigned char memory[0x10000]; // 64K address space
};

union address_space address_space;

// make address space properties global, so we can use them directly
unsigned char* memory = address_space.memory;
unsigned char* ioports = address_space.ioports;
unsigned char* interrupt_request_register = address_space.interrupt_request_register;

// Gameboy game read only memory (inserted cartridge)
unsigned char rom[0x200000];

// Gameboy bootstrap read only memory (hardcoded bootstrap)
unsigned char bootrom[256];


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

