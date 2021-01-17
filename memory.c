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
 *  > Memory map
 *  http://gameboy.mongenel.com/dmg/asmmemmap.html
 *
 */
#include <string.h>

// Gameboy address space (RAM + VRAM?)
union address_space {
    struct {
        union {
            struct {
                unsigned char rombank_fixed[0x4000]; // 16k for ROM Bank 0
                unsigned char rombank_switchable[0x4000]; // 16k for Switchable ROM Bank (1..N)
            };
            unsigned char rombanks[0x8000];
        };
        unsigned char vram[0x2000]; // 8K for VRAM
        unsigned char external_ram[0x2000]; // 8K for External Switchable RAM in Cartridge
        unsigned char ram[0x2000]; // 8K for Internal Work RAM
        unsigned char echo[0x1E00]; // Echo of Internal RAM
        unsigned char oam[0xA0]; // Object Attribute Memory (Sprite Attribute Table)
        unsigned char unusable[0x60]; // Not usable
        union {
            struct {
                unsigned char padding[0x4];
                unsigned char tdiv[1]; // Timer divider register $FF04 incremented at 16384Hz
                unsigned char tima[1]; /* Timer counter register $FF05 incremented at frequency in TAC 
                                          Requests interrupt when it overflows */
                unsigned char tma[1]; // Timer modulo $FF06
                unsigned char tac[1]; // Timer control $FF07 Bit 1-0 sets frequency, bit 2 enables/disables
                unsigned char padding____[7];
                unsigned char interrupt_request_register[1]; // Interrupt request register ($FF0F)
                unsigned char padding_[0x30];
                unsigned char lcdc[1]; // LCD Control ($FF40)
                unsigned char lcdc_stat[1]; // LCD Control Status ($FF41)
                unsigned char lcd_scy[1]; // LCD Scroll Y ($FF42)
                unsigned char lcd_scx[1]; /* LCD Scroll X ($FF43)
                                           *
                                           * (Scroll Y and X are used to determine the position of background (256x256)
                                           * to be displayed at the upper/left LCD display position)
                                           *
                                           * (Used for scrolling background map)
                                           */
                unsigned char lcd_ly[1]; /* LCDC Y-Coordinate ($FF44)
                                          *
                                          * (Indicates vertical line of the LCD Driver to where present data is being transferred)
                                          *
                                          * Can take any value between 0-153: Values from 144-153 indicate the V-Blank Period
                                          */
                unsigned char lcd_lyc[1]; /* LCD LY Compare ($FF45)
                                           * 
                                           * Gameboy permanently compares LYC and LY. When both are identical, 
                                           * the coincident bit in the STAT becomes set,
                                           * and (if enabled) a STAT interrupt is requested
                                           */
                unsigned char dma[1]; // DMA Transfer and Start Address ($FF46)
                unsigned char lcd_bgp[1]; /* BG Pallete Data ($FF47)
                                       * 
                                       * Assigns gray shades to the color number of the BG
                                       *
                                       * Bit 7-6 - Shade color for Color Number 3
                                       * Bit 5-4 - Shade color for Color Number 2
                                       * etc...
                                       */
                unsigned char obj_pallete_0_data[1]; // pallete 0 for sprites ($FF48)
                unsigned char obj_pallete_1_data[1]; // pallete 1 for sprites ($FF49)
                unsigned char lcd_windowy[1]; // window y location in relation to scroll (viewport) ($FF4A)
                unsigned char lcd_windowx[1]; // window x location in relation to scroll (viewport) ($FF4B)
                unsigned char padding__[0x4]; // $FF4C - $FF4F
                unsigned char disabled_bootrom[1]; // $FF50
                unsigned char padding___[0x2F];
            };
            unsigned char ioports[0x80]; // IO Ports ($FF00)
        };
        unsigned char hram[0x7F]; // High RAM
        unsigned char interrupt_enable_register[1]; // Interrupt enable register
    };
    unsigned char memory[0x10000]; // 64K address space
};

union address_space address_space;

// make address space properties global, so we can use them directly
unsigned char* memory = address_space.memory;
unsigned char* ioports = address_space.ioports;
unsigned char* interrupt_request_register = address_space.interrupt_request_register;
unsigned char* interrupt_enable_register = address_space.interrupt_enable_register;
unsigned char* lcdc = address_space.lcdc;
unsigned char* lcdc_stat = address_space.lcdc_stat;
unsigned char* lcd_ly = address_space.lcd_ly;
unsigned char* lcd_lyc = address_space.lcd_lyc;
unsigned char* lcd_scy = address_space.lcd_scy;
unsigned char* lcd_scx = address_space.lcd_scx;
unsigned char* lcd_windowy = address_space.lcd_windowy;
unsigned char* lcd_windowx = address_space.lcd_windowx;
unsigned char* lcd_bgp = address_space.lcd_bgp;
unsigned char* rombanks = address_space.rombanks;
unsigned char* disabled_bootrom = address_space.disabled_bootrom;
unsigned char* tdiv = address_space.tdiv;
unsigned char* tima = address_space.tima;
unsigned char* tma = address_space.tma;
unsigned char* tac = address_space.tac;

// Gameboy game read only memory (inserted cartridge)
unsigned char rom[0x200000];

unsigned char cartridge_loaded = 0; // 0 if cartridge isn't loaded, 1 if it's partially loaded (all except first 256 bytes), 2 if it's fully loaded

static void load_bootstrap_rom() {

    FILE* bootstrap = fopen("bootstrap_rom", "rb");
    fread(memory, sizeof(unsigned char), 256, bootstrap);

    fclose(bootstrap);
}

void insert_cartridge(char* filename) {

    FILE* cartridge = fopen(filename, "rb");
    fread(rom, sizeof(unsigned char), 0x200000, cartridge);

    fclose(cartridge);

    cartridge_loaded = 1;
}

void load_roms() {
    load_bootstrap_rom();

    if (!cartridge_loaded)
        memset(rom, 0xff, 0x200000);

    memcpy(memory+256, rom+256, 0x8000-256);
}

void load_tests(char* testpath) {

    FILE* test = fopen(testpath, "rb");
    fread(memory, sizeof(unsigned char), 0x8000, test);

    fclose(test);

}

void check_disable_bootrom() {

    if (cartridge_loaded == 1 && *disabled_bootrom) {

        cartridge_loaded++;
        memcpy(memory, rom, 256);
    }

}
