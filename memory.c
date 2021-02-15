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
#include <assert.h>

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
unsigned char ram_banks[0x8000] = {0}; // Max 4 ram banks (only 2 bits to change it), RAM can be 2KB, 8KB or 32KB (in the form of 4 8KB banks)

unsigned char mbctype = 0;
unsigned char romsizetype = 0;
unsigned char ramsizetype = 0;

unsigned char ram_enable_register = 0;
unsigned char rom_bank_number = 1; // 5 bit register selects ROM bank number
unsigned char ram_or_upperrom_bank_number = 0; // 2 bits register selects ROM bank number upper 2 bits or RAM bank number
unsigned char banking_mode_select = 0; // 1 bit register selects between two MBC1 banking modes (mode 0 or 1)


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
        mbctype = memory[0x147];
        romsizetype = memory[0x148];
        ramsizetype = memory[0x149];
        
        printf("Loaded cartridge.\n");

        printf("MBC TYPE %d\n", mbctype);
        printf("ROM SIZE TYPE%d\n", romsizetype);
        printf("RAM SIZE TYPE%d\n", ramsizetype);

    }

}

void dma_transfer(unsigned char data) {

    /* printf("DMA transfer\n"); */

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

        // Handle Bank changing

        // Enable RAM Banking
        if (address < 0x2000 && mbctype >= 1 && mbctype <= 3) {

            // MBC1
            if (mbctype >= 1 && mbctype <= 3) {

                // RAM Banking will be enabled when the lower nibble is 0xA, and disabled when the lower nibble is 0
                if ((data & 0xF) == 0xA) {

                    printf("Enabled RAM\n");
                    ram_enable_register = 1;
                }
                else {

                    printf("Disabled RAM\n");
                    ram_enable_register = 0;
                }

            }

        }
        // Change ROM Bank
        else if (address >= 0x2000 && address < 0x4000) {

            // MBC1
            if (mbctype >= 1 && mbctype <= 3) {

                // Write to rom_bank_number (5bit register from 0x1 - 0x1F)

                printf("Writing to ROM BANK LOWER BITS (%x) with data %x\n", rom_bank_number, data);

                data &= 0x1f; // number of bits on the register is just 5
                
                unsigned char newdata = data;

                if (romsizetype == 0) // no banking - bank number should be always 1
                    newdata = 1;
                else if (romsizetype == 1) // 4 banks can be addressed with 2 bits
                    newdata &= 3; // 0..0011
                else if (romsizetype == 2) // 8 banks can be addressed with 3 bits
                    newdata &= 7; // 0..0111
                else if (romsizetype == 3) // 16 banks
                    newdata &= 0xf; // 0..1111
                else
                    newdata &= 0x1f; // 0001 1111
                
                rom_bank_number = data % 0x20 != 0 ? newdata : 1; // becomes 1 if the data written to the 5 bits of the register was 0

                printf("ROM BANK LOWER BITS is now %x\n", rom_bank_number);
            }

        }
        // Change ROM Bank upper part or RAM bank
        else if (address >= 0x4000 && address < 0x6000) {

            printf("Writing to ROM BANK UPPER BITS (%x) with data %x\n", ram_or_upperrom_bank_number, data);

            // MBC1
            if (mbctype >= 1 && mbctype <= 3) {

                // 2 bit register selects RAM Bank or specify two upper bits of the ROM Bank number
                // as long as one of either RAM or ROM is large enough

                if (romsizetype > 4 || ramsizetype == 3) {

                    ram_or_upperrom_bank_number = data & 3;
                }

            }

        }
        // Change *changing* to ROM Bank or RAM Bank
        else if (address >= 0x6000 && address < 0x8000) {

            // MBC1
            if (mbctype >= 1 && mbctype <= 3) {

                // is a 1 bit register

                banking_mode_select = data & 1; 

                // For Large RAM cart, switching to mode 1 enables RAM Banking and switches the bank selected
                // For Large ROM carts, switching to mode 1 makes bank 0x20, 0x40, 0x60 bankable through 0000-3fff

            }

        }

        return extra_cycles;
    }
    else if (address >= 0xa000 && address < 0xc000) {

        // Writing to RAM Bank address

        if (ramsizetype >= 3 && ram_enable_register == 1 && banking_mode_select == 1) {

            unsigned char current_ram_bank = ram_or_upperrom_bank_number;
            ram_banks[(address - 0xa00) + current_ram_bank*0x2000] = data;

            return extra_cycles;
        }

        // If there's no ram or if it's disabled don't allow writing
        else if (ramsizetype == 0 || ram_enable_register == 0)
            return extra_cycles;

    }

    // Other
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

        // if one of the bits is changing the input type, reset pressed keys
        /* if ((data & 0x10 && !(*joyp & 0x10)) || */
        /*         (data & 0x20 && !(*joyp & 0x20))) { */

        /*     // Reset lower 4 bits to reset inputs */
        /*     data |= 0xF; */
        /* } */

        // I guess the code above doesn't work, and i actually need to reset the lower nibble everytime $ff00 is written...
        data |= 0xF;

        // Set data directly to joypad (this will set bit 4 and 5) along with the reset controls if that was the case
        *joyp = data;

        return extra_cycles;

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
    else if (&memory[address] == joyp) { // $0xFF00

        
        *destination = *joyp; 

        return;
    }
    else if (address == 0xFF4D) {

        // When reading speed switching for GBC just return FF to avoid bugs? (i'm not doing GBC)

        *destination = 0xFF;
        return;

    } 

    // MBC Control
    else if (address < 0x4000) {

        // MBC1 in mode 1 with large ROM (>=1MB)
        if (mbctype >= 1 && mbctype <= 3 && banking_mode_select == 1 && romsizetype > 4) {

            // TODO: Multicarts MBC1m use one less bit in rom bank number and shift by 4 only
            unsigned char current_rom_bank = rom_bank_number | (ram_or_upperrom_bank_number << 5);

            if (current_rom_bank % 0x20 == 0) {

                // Access bank 00, 20h, 40h or 60h

                printf("Accessing bank %x at %x\n", current_rom_bank, address + current_rom_bank*0x4000);
                *destination = rom[address + current_rom_bank*0x4000]; // 16K banks
                return;
            }
            
        }

    }
    else if (address >= 0x4000 && address < 0x8000) {

        // Reading from ROM Bank
    
        // MBC1
        if (mbctype >= 1 && mbctype <= 3) {

            // TODO: Multicarts MBC1m use one less bit in rom bank number and shift by 4 only
            unsigned char current_rom_bank = rom_bank_number | (ram_or_upperrom_bank_number << 5);

            // Can't be 00h 20h 40h or 60h, automatically access next bank
            /* if(current_rom_bank % 20 == 0) */
            /*     current_rom_bank++; */


            // Since rom_bank is always 1 or more, we remove 0x4000 once to get the correct offset
            /* printf("Accessing bank %x at %x\n", current_rom_bank, address + current_rom_bank*0x4000); */
            *destination = rom[(address - 0x4000) + current_rom_bank*0x4000];
            return;

        }

    }
    else if (address >= 0xA000 && address < 0xC000) {

        // Reading from RAM Bank
       
        // MBC1 with RAM and more than 1 8K RAM Bank
        if (mbctype >= 1 && mbctype <= 3) {

            // No RAM
            if (ramsizetype == 0) {
                // No RAM returns undefined when reading (usually 0xFF)
                *destination = 0xFF;
                return;
            }

            // RAM size 2KB
            else if (ramsizetype == 1 && address - 0xA000 >= 0x2000) {

                // accessing outside the 2K of RAM is undefined when reading (usually 0xFF)
                *destination = 0xFF;
                return;
            }

            // When RAM is within 2KB or is 8KB, access space normally as if it was the 1 bank dedicated

            // Big RAM and mode is 1 and if ram is enabled
            else if (ramsizetype >= 3 && banking_mode_select == 1 && ram_enable_register == 1) {

                // multiple RAM Banks - address with rambank selector 

                unsigned char current_ram_bank = ram_or_upperrom_bank_number;
                *destination = ram_banks[(address - 0xa00) + current_ram_bank*0x2000];
                return;

            }
                

        }

    }

    *destination = memory[address];

}
