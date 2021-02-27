#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "memory.h"

static union address_space address_space;

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
unsigned char* obj_palette_0_data = address_space.obj_palette_0_data;
unsigned char* obj_palette_1_data = address_space.obj_palette_1_data;

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

    // TODO: I probably can't do this
    unsigned char bootstrap_rom[] = {
        0x31, 0xfe, 0xff, 0xaf, 0x21, 0xff, 0x9f, 0x32, 0xcb, 0x7c, 0x20, 0xfb,
        0x21, 0x26, 0xff, 0x0e, 0x11, 0x3e, 0x80, 0x32, 0xe2, 0x0c, 0x3e, 0xf3,
        0xe2, 0x32, 0x3e, 0x77, 0x77, 0x3e, 0xfc, 0xe0, 0x47, 0x11, 0x04, 0x01,
        0x21, 0x10, 0x80, 0x1a, 0xcd, 0x95, 0x00, 0xcd, 0x96, 0x00, 0x13, 0x7b,
        0xfe, 0x34, 0x20, 0xf3, 0x11, 0xd8, 0x00, 0x06, 0x08, 0x1a, 0x13, 0x22,
        0x23, 0x05, 0x20, 0xf9, 0x3e, 0x19, 0xea, 0x10, 0x99, 0x21, 0x2f, 0x99,
        0x0e, 0x0c, 0x3d, 0x28, 0x08, 0x32, 0x0d, 0x20, 0xf9, 0x2e, 0x0f, 0x18,
        0xf3, 0x67, 0x3e, 0x64, 0x57, 0xe0, 0x42, 0x3e, 0x91, 0xe0, 0x40, 0x04,
        0x1e, 0x02, 0x0e, 0x0c, 0xf0, 0x44, 0xfe, 0x90, 0x20, 0xfa, 0x0d, 0x20,
        0xf7, 0x1d, 0x20, 0xf2, 0x0e, 0x13, 0x24, 0x7c, 0x1e, 0x83, 0xfe, 0x62,
        0x28, 0x06, 0x1e, 0xc1, 0xfe, 0x64, 0x20, 0x06, 0x7b, 0xe2, 0x0c, 0x3e,
        0x87, 0xe2, 0xf0, 0x42, 0x90, 0xe0, 0x42, 0x15, 0x20, 0xd2, 0x05, 0x20,
        0x4f, 0x16, 0x20, 0x18, 0xcb, 0x4f, 0x06, 0x04, 0xc5, 0xcb, 0x11, 0x17,
        0xc1, 0xcb, 0x11, 0x17, 0x05, 0x20, 0xf5, 0x22, 0x23, 0x22, 0x23, 0xc9,
        0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83,
        0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
        0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63,
        0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e,
        0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c, 0x21, 0x04, 0x01, 0x11,
        0xa8, 0x00, 0x1a, 0x13, 0xbe, 0x20, 0xfe, 0x23, 0x7d, 0xfe, 0x34, 0x20,
        0xf5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xfb, 0x86, 0x20, 0xfe,
        0x3e, 0x01, 0xe0, 0x50
    };
    unsigned int bootstrap_rom_len = 256;

    memcpy(memory, bootstrap_rom, bootstrap_rom_len);

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

static void dma_transfer(unsigned char data) {

    /* printf("DMA transfer\n"); */

    // The written data value specifies the transfer source address divided by 0x100
    // By multiplying by 0x100 (which is << 8) we get the source address
    // Read from source address XX00 until XX9F (XX is defined by data)
    // And write to FE00 until FE9F
    unsigned short address = data << 8;
    for (int i = 0; i < 0xA0; i++) {

        unsigned char aux;
        mmu_read8bit(&aux, address + i);
        mmu_write8bit(0xfe00 + i, aux);
        /* printf("DMA transfer to address %x from address %x with data %x\n", 0xfe00+i, address+i, aux); */
    }

}


int mmu_write8bit(unsigned short address, unsigned char data) {
    

    /* printf("Write address %x\n", address); */

    int extra_cycles = 0;

    if (address < 0x8000) {

        /* printf("Writing to ROM RamEnable: %x Bank1: %x Bank2: %x ModeSelect: %x with Address: %x and data %x\n", ram_enable_register, rom_bank_number, ram_or_upperrom_bank_number, banking_mode_select, address, data); */
        

        // Read only memory

        // When the game writes to the ROM addresses (here), it is trapped and decyphered to change the Banks

        // Handle Bank changing

        // Enable RAM Banking
        if (address < 0x2000 && mbctype >= 1 && mbctype <= 3) {

            // MBC1
            if (mbctype >= 1 && mbctype <= 3) {

                // RAM Banking will be enabled when the lower nibble is 0xA, and disabled when the lower nibble is 0
                if ((data & 0xF) == 0xA) {

                    /* printf("Enabled RAM\n"); */
                    ram_enable_register = 1;
                }
                else {

                    /* printf("Disabled RAM\n"); */
                    ram_enable_register = 0;
                }

            }

        }
        // Change ROM Bank
        else if (address >= 0x2000 && address < 0x4000) {

            // MBC1
            if (mbctype >= 1 && mbctype <= 3) {

                // Write to rom_bank_number (5bit register from 0x1 - 0x1F)

                /* printf("Writing to ROM BANK LOWER BITS (%x) with data %x\n", rom_bank_number, data); */

                data &= 0x1f; // number of bits on the register is just 5

                // the 5 bit BANK1 register doesn't allow value 0 -> write 1 instead
                rom_bank_number = data ? data : 1;


                /* printf("ROM BANK LOWER BITS is now %x\n", rom_bank_number); */
            }

        }
        // Change ROM Bank upper part or RAM bank
        else if (address >= 0x4000 && address < 0x6000) {

            /* printf("Writing to ROM BANK UPPER BITS (%x) with data %x\n", ram_or_upperrom_bank_number, data); */

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

    /* printf("Write to  %X: %02X\n", address, data); */


        return extra_cycles;
    }
    else if (address >= 0xa000 && address < 0xc000) {

        /* printf("Writing to SRAM RamEnable: %x Bank1: %x Bank2: %x ModeSelect: %x\n", ram_enable_register, rom_bank_number, ram_or_upperrom_bank_number, banking_mode_select); */

        // Writing to RAM Bank address - only if it exists and is enabled

        // MBC1 and Enabled RAMG
        if (mbctype >= 1 && mbctype <= 3 && ram_enable_register) {

            // No RAM
            if (ramsizetype == 0) {
                // No RAM returns undefined when reading (usually 0xFF)
                // (return is done outside the if chain below)
            }

            // RAM size 2KB might be accessed outside that space, but
            // accessing outside the 2K of RAM is undefined behaviour, so we just let it

            // RAM size is 2KB or 8KB within boundaries
            else if (ramsizetype == 1 || ramsizetype == 2) {

                /* printf("Writing to SRAM address %x DATA %x\n", address - 0xa000, data); */
                ram_banks[address - 0xa000] = data;

                /* // Disable SRAM after writing to it */
                /* printf("Disabled SRAM\n"); */
                /* ram_enable_register = 0; */

    /* printf("Write to  %X: %02X\n", address, data); */
                return extra_cycles;
            }
            // Big RAM and mode is 1 and if ram is enabled
            else if (ramsizetype >= 3) {

                if (banking_mode_select == 1) {

                    // banking select is mode 1 - address with rambank selector
                    unsigned char current_ram_bank = ram_or_upperrom_bank_number;
                    ram_banks[(address - 0xa000) + current_ram_bank*0x2000] = data;
                }
                else {

                    // when banking select mode is 0 - access first bank
                    ram_banks[(address - 0xa000)] = data;
                }

                /* // Disable SRAM after writing to it */
                /* printf("Disabled SRAM\n"); */
                /* ram_enable_register = 0; */
                
    /* printf("Write to  %X: %02X\n", address, data); */
                return extra_cycles;

            }

        }

        // Extra RAM couldn't be written
    /* printf("Write to  %X: %02X\n", address, data); */
        return extra_cycles;

    }

    // Other
    else if (address >= 0xfea0 && address < 0xfeff) {

        // Unusable area
    /* printf("Write to  %X: %02X\n", address, data); */
        return extra_cycles;
    }
    else if (address == 0xFF00) {


        // Writing to joypad

        // TODO: This might not be correct, and maybe instead when reading FF00 i should
        // get the correct keys pressed from joypad_state variable in ppu.c


        // Assuming I could do it like this

        // When writing to joypad and *effectively changing* type from input to standard, all pressed buttons must be reset

        // I guess the code (that used to be) above doesn't work, and i actually need to reset the lower nibble everytime $ff00 is written...
        data |= 0xF;

        // Set data directly to joypad (this will set bit 4 and 5) along with the reset controls if that was the case
        *joyp = data;

    /* printf("Write to  %X: %02X\n", address, data); */
        return extra_cycles;

    }
    else if (&memory[address] == tdiv) {

        // If you try writing to DIV, it'll be set to 0
        memory[address] = 0;
    /* printf("Write to  %X: %02X\n", address, data); */
        return extra_cycles;
    }
    else if (&memory[address] == dma) {
        
        // Writing to DMA Transfer and Start address

        // Launch a DMA transfer from ROM to OAM
        /* printf("DOING DMA TRANSFER\n"); */
        dma_transfer(data);
        extra_cycles = 160;
    }
    else if (address <= 0x9fff && address >= 0x8000) {

        // Destination is VRAM

        unsigned char mode = *lcdc_stat & 3; // 3 = 0b11, get first two bits that define the mode

        if (mode == 3) {

            // During LCDC mode 3, no data can be written to VRAM

    /* printf("Write to  %X: %02X\n", address, data); */
            return extra_cycles;
        }
    }
    else if (address <= 0xfe9f && address >= 0xfe00) {

        // Destination is OAM

        unsigned char mode = *lcdc_stat & 3; // 3 = 0b11, get first two bits that define the mode

        if (mode > 1) {

            // During LCDC mode 2 and 3, no data can be written to OAM

    /* printf("Write to  %X: %02X\n", address, data); */
            return extra_cycles;
        }

    }

    assert(!(address >= 0xa000 && address < 0xc000));

    memory[address] = data;

    /* printf("Write to  %X: %02X\n", address, data); */
    return extra_cycles;
}

void mmu_read8bit(unsigned char* destination, unsigned short address) {

    if (address <= 0x9fff && address >= 0x8000) {

        // Reading from VRAM

        unsigned char mode = *lcdc_stat & 3; // 3 = 0b11, get first two bits that define the mode

        if (mode == 3) {

            // During LCDC mode 3, no data can be read from VRAM

            *destination = 0xFF; // Read undefined value which is usually 0xFF
    /* printf("Read from %X: %02X\n", address, *destination); */

            return;
        }
    }
    else if (address <= 0xfe9f && address >= 0xfe00) {

        // Reading from OAM

        unsigned char mode = *lcdc_stat & 3; // 3 = 0b11, get first two bits that define the mode

        if (mode > 1) {

            // During LCDC mode 2 and 3, no data can be read from OAM

            *destination = 0xFF; 
    /* printf("Read from %X: %02X\n", address, *destination); */

            return;
        }

    }
    else if (address >= 0xe000 && address < 0xfe00) {

        // Reading from ECHO ram mirrors RAM
        *destination = memory[address-0x2000];

    }
    else if (&memory[address] == joyp) { // $0xFF00

        
        *destination = *joyp; 
    /* printf("Read from %X: %02X\n", address, *destination); */


        return;
    }
    else if (address == 0xFF4D) {

        // When reading speed switching for GBC just return FF to avoid bugs? (i'm not doing GBC)

        *destination = 0xFF;
    /* printf("Read from %X: %02X\n", address, *destination); */

        return;

    } 

    // MBC Control
    else if (address < 0x4000) {

        // MBC1 in mode 1 with large ROM (>=1MB)
        if (mbctype >= 1 && mbctype <= 3 && banking_mode_select == 1 && romsizetype > 4) {

            // TODO: Multicarts MBC1m use one less bit in rom bank number and shift by 4 only

            // If 0000-3fff is accessed in mode 1, bank is upper bank shifted by 5
            // To Access bank 00, 20h, 40h or 60h

            unsigned char max_banks_mask = ~(-1 << (romsizetype+1));
            unsigned char current_rom_bank = (ram_or_upperrom_bank_number << 5) & max_banks_mask;

            /* printf("Accessing SPECIAL bank %x at %x\n", current_rom_bank, address + current_rom_bank*0x4000); */

            *destination = rom[address + current_rom_bank*0x4000]; // 16K banks
    /* printf("Read from %X: %02X\n", address, *destination); */

            return;
            
        }

    }
    else if (address >= 0x4000 && address < 0x8000) {

        // Reading from ROM Bank

        /* printf("Reading from ROM RamEnable: %x Bank1: %x Bank2: %x ModeSelect: %x\n", ram_enable_register, rom_bank_number, ram_or_upperrom_bank_number, banking_mode_select); */
    
        // MBC1
        if (mbctype >= 1 && mbctype <= 3) {

            // TODO: Multicarts MBC1m use one less bit in rom bank number and shift by 4 only
            unsigned char max_banks_mask = ~(-1 << (romsizetype+1));

            unsigned char current_rom_bank = ((rom_bank_number ? rom_bank_number : 1) | (ram_or_upperrom_bank_number << 5)) & max_banks_mask;


            // Since rom_bank is always 1 or more, we remove 0x4000 once to get the correct offset
            /* printf("Accessing bank %x at %x\n", current_rom_bank, address + current_rom_bank*0x4000); */
            *destination = rom[(address - 0x4000) + current_rom_bank*0x4000];
    /* printf("Read from %X: %02X\n", address, *destination); */

            return;

        }

    }
    else if (address >= 0xA000 && address < 0xC000) {

        /* printf("Reading from SRAM\n"); */

        /* printf("Reading from SRAM RamEnable: %x Bank1: %x Bank2: %x ModeSelect: %x\n", ram_enable_register, rom_bank_number, ram_or_upperrom_bank_number, banking_mode_select); */

        // Reading from RAM Bank
       
        // MBC1 with RAM and more than 1 8K RAM Bank
        // RAMG MUST be enabled, or else undefined is read 
        if (mbctype >= 1 && mbctype <= 3 && ram_enable_register) {

            /* printf("RAM GETS PAST MBC CHECK\n"); */

            // No RAM
            if (ramsizetype == 0) {
                // No RAM returns undefined when reading (usually 0xFF)
                // (return is done outside the if chain below)
                /* printf("SRAM READ IS LOST IN FIRST IF CHECK\n"); */
            }

            // RAM size 2KB might be accessed outside that space, but
            // accessing outside the 2K of RAM is undefined behaviour, so we just let it

            // RAM size is 2KB or 8KB within boundaries
            else if (ramsizetype == 1 || ramsizetype == 2) {

                /* printf("Reading from correct SRAM address: %x\n", address - 0xa000); */

                *destination = ram_banks[address - 0xa000];
    /* printf("Read from %X: %02X\n", address, *destination); */

                return;
            }
            // Big RAM and mode is 1 and if ram is enabled
            else if (ramsizetype >= 3) {

                /* printf("SRAM READ IS NOT LOST IN LAST IF CHECK\n"); */

                if (banking_mode_select == 1) {

                    // banking select is mode 1 - address with rambank selector
                    unsigned char current_ram_bank = ram_or_upperrom_bank_number;
                    *destination = ram_banks[(address - 0xa000) + current_ram_bank*0x2000];
                }
                else {

                    // when banking select mode is 0 - access first bank
                    *destination = ram_banks[(address - 0xa000)];
                }

    /* printf("Read from %X: %02X\n", address, *destination); */

                return;

            }
                
        }

        /* printf("Returning UNDEFINED VALUE FOR SRAM ACCESS\n"); */

        // When no Cartridge RAM is present, or is disabled, return undefined
        *destination = 0xFF;
    /* printf("Read from %X: %02X\n", address, *destination); */

        return;

    }

    assert(!(address >= 0xA000 && address < 0xC000));

    *destination = memory[address];
    /* printf("Read from %X: %02X\n", address, *destination); */


}
