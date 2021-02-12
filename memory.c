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
                unsigned char joyp[1];
                unsigned char padding[0x3];
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
unsigned char* dma = address_space.dma;
unsigned char* joyp = address_space.joyp;

// Gameboy game read only memory (inserted cartridge)
unsigned char rom[0x200000] = {0};
unsigned char ram_banks[0x8000] = {0}; // Max 4 ram banks (only 2 bits to change it), each has 0x2000 bytes

unsigned char mbctype = 0;

// Which ROM bank is currently loaded in address [0x4000 - 0x7fff]
// Starts at one because rombank 0 is fixed in address [0x0 - 0x4000]
unsigned char current_rom_bank = 1;
// Which RAM bank is currently loaded in address []
unsigned char current_ram_bank = 0;
unsigned char rambanks_enabled = 0;
unsigned char rambanks_can_be_enabled = 0;
unsigned char changing_rombank = 1; // Doing ROM Bank or RAM Bank change?

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
    fread(rom, sizeof(unsigned char), 0x200000, test);
    memcpy(memory, rom, 0x8000);

    fclose(test);

    *disabled_bootrom = 1;
}

void check_disable_bootrom() {

    if (cartridge_loaded == 1 && *disabled_bootrom) {

        cartridge_loaded++;
        // Replace bootrom in memory with game rom data
        memcpy(memory, rom, 256);

        // Read the MBC type
        switch (memory[0x147]) {
            case 1:
            case 2:
                rambanks_can_be_enabled = 1;
            case 3:
                mbctype = 1;
                break;
            case 5:
            case 6:
                mbctype = 2;
                break;
        }
        
        printf("MBC TYPE %d\n", mbctype);

    }

}

void dma_transfer(unsigned char data) {

    printf("DMA transfer\n");

    // The written data value specifies the transfer source address divided by 0x100
    // By multiplying by 0x100 (which is << 8) we get the source address
    // Read from source address XX00 until XX9F (XX is defined by data)
    // And write to FE00 until FE9F
    unsigned short address = data << 8;
    for (int i = 0; i < 0xA0; i++)
         memory[0xfe00 + i] = memory[address + i];

}


int mmu_write8bit(unsigned short address, unsigned char data) {
    
    /* printf("Write address %x\n", address); */

    int extra_cycles = 0;

    if (address < 0x8000) {
        // Read only memory

        // When the game writes to the ROM addresses (here), it is trapped and decyphered to change the Banks

        printf("Write ROM address %x\n", address);

        // Handle Bank changing

        // Enable RAM Banking
        if (address < 0x2000 && (mbctype == 1 || mbctype == 2)) {

            // MBC 2 is the same as for MBC1 but the 4th bit must be 0, if it's 1 do nothing
            if (mbctype == 2 && data & 8)
                return extra_cycles;

            // RAM Banking will be enabled when the lower nibble is 0xA, and disabled when the lower nibble is 0
            if ((data & 0xF) == 0xa)
                rambanks_enabled = rambanks_can_be_enabled;
            else
                rambanks_enabled = 0;

        }
        // Change ROM Bank
        else if (address >= 0x2000 && address < 0x4000 && (mbctype == 1 || mbctype == 2)) {

            // Change bits 0-4 of the current rom bank number in MBC1 or bits 0-3 in MBC2 

            printf("Changing ROM Bank with data %d\n", data);

            if (mbctype == 2) {
                // lower nibble (first 4 bits) sets rom bank
                // it can never be 0 bc rom bank 0 is fixed in 0-0x4000. We treat 0 as rom bank 1
                current_rom_bank = data & 0xF ? data & 0xF : 1;

            }
            else if (mbctype == 1) {

                // clear and then set 5 lower bits
                
                current_rom_bank &= 0xE0; // 1110 0000
                current_rom_bank |= data & 0x1F; // 0001 1111
                current_rom_bank = current_rom_bank ? current_rom_bank : 1; // Can't be 0 
            }


        }
        // Change ROM Bank upper part or RAM bank
        else if (address >= 0x4000 && address < 0x6000 && mbctype == 1) {

            printf("Changing ROM bank upper bits %x\n", address);

            // If doing ROM Bank change set upper 3 bits

            if (changing_rombank) {

                // Do ROM - 2 upper bits - change

                current_rom_bank &= 0x1F; // Keep only lower 5 bits
                current_rom_bank |= (data & 0x3) << 5; // Add additional upper 2 bits from the lower byte of the address
                current_rom_bank = current_rom_bank ? current_rom_bank : 1; // Can't be 0
            }
            else {
            
                // Do RAM change
                current_ram_bank = data & 0x3; // Ram bank is selected 
            }

        }
        // Change *changing* to ROM Bank or RAM Bank
        else if (address >= 0x6000 && address < 0x8000 && mbctype == 1) {

            printf("Changing changing ROM or RAM bank %d\n", address);

            // The lowest bit defines whether it's ROM or RAM changing
            // If it's 0 -> ROM, 1 -> RAM
            changing_rombank = !(data & 1);

            // When changing ROM Bank set RAM Bank to 0 bc the gameboy can only use rambank 0 in this mode
            if (changing_rombank)
                current_ram_bank = 0;

        }
        

        return extra_cycles;
    }
    else if (address >= 0xa000 && address < 0xc000) {

        // Writing to RAM Bank address

        if (rambanks_enabled) {

            ram_banks[(address - 0xa00) + current_ram_bank*0x2000] = data;
        }

        return extra_cycles;
    }
    else if (address >= 0xfea0 && address < 0xfeff) {

        // Unusable area
        return extra_cycles;
    }
    else if (address == 0xFF00) {

        // Writing to joypad

        // TODO: This might not be correct, and maybe instead when reading FF00 i should
        // get the correct keys pressed from joypad_state variable in ppu.c


        // Assuming I could do it like this

        // When writing to joypad and *effectively changing* type from input to standard, all pressed buttons must be reset

        // bit 4 selects direction keys
        // bit 5 selects button keys
        if ((data & 0x10 && !(*joyp & 0x10)) ||
                (data & 0x20 && !(*joyp & 0x20))) {

            // Clear lower 4 bits to reset inputs
            data &= 0xF0;

        }

    }
    else if (&memory[address] == tdiv) {

        // If you try writing to DIV, it'll be set to 0
        memory[address] = 0;
    }
    else if (&memory[address] == dma) {
        
        // Writing to DMA Transfer and Start address

        // Launch a DMA transfer from ROM to OAM
        dma_transfer(data);
        extra_cycles = 160;
    }
    else if (address <= 0x9fff && address >= 0x8000) {

        // Destination is VRAM

        unsigned char mode = *lcdc_stat & 3; // 3 = 0b11, get first two bits that define the mode

        if (mode == 3) {

            // During LCDC mode 3, no data can be written to VRAM

            return extra_cycles;
        }
    }
    else if (address <= 0xfe9f && address >= 0xfe00) {

        // Destination is OAM

        unsigned char mode = *lcdc_stat & 3; // 3 = 0b11, get first two bits that define the mode

        if (mode > 1) {

            // During LCDC mode 2 and 3, no data can be written to OAM

            return extra_cycles;
        }

    }
    else if (address >= 0xe000 && address < 0xfe00) {

        // Writing to ECHO ram also writes in RAM

        memory[address - 0x2000] = data;

    }


    memory[address] = data;

    return extra_cycles;
}

void mmu_read8bit(unsigned char* destination, unsigned short address) {

    /* printf("Read address %x\n", address); */


    if (address <= 0x9fff && address >= 0x8000) {

        // Reading from VRAM

        unsigned char mode = *lcdc_stat & 3; // 3 = 0b11, get first two bits that define the mode

        if (mode == 3) {

            // During LCDC mode 3, no data can be read from VRAM

            *destination = 0xFF; // Read undefined value which is usually 0xFF
            return;
        }
    }
    else if (address <= 0xfe9f && address >= 0xfe00) {

        // Reading from OAM

        unsigned char mode = *lcdc_stat & 3; // 3 = 0b11, get first two bits that define the mode

        if (mode > 1) {

            // During LCDC mode 2 and 3, no data can be read from OAM

            *destination = 0xFF; 
            return;
        }

    }
    else if (address == 0xFF4D) {

        // When reading speed switching for GBC just return FF to avoid bugs? (i'm not doing GBC)

        *destination = 0xFF;
        return;

    }
    else if (address >= 0x4000 && address < 0x8000) {

        // Reading from ROM Bank
        
        // Read from current rom bank
        /* printf("Reading from 0x4000-0x8000, current rom bank is %d, desired address was %x, actual address is %x\n", current_rom_bank, address, (address - 0x4000) + current_rom_bank*0x4000); */
    
        // Since rom_bank is always 1 or more, we remove 0x4000 once to get the correct offset
        *destination = rom[(address - 0x4000) + current_rom_bank*0x4000];
        return;
    }
    else if (address >= 0xA000 && address < 0xC000 && mbctype > 0 && rambanks_enabled) {

        // Reading from RAM Bank
       
        printf("reading from RAM BANK\n");
        
        // Read from current ram bank
        // rebase address and offset to the correct ram bank (each has 0x2000 size)
        *destination = ram_banks[(address - 0xa00) + current_ram_bank*0x2000];
        return;
    }

    *destination = memory[address];

}
