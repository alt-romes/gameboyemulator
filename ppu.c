/*
 * Gameboy Emulator: Pixel Processing Unit
 *
 * Resources:
 *
 * > General 
 * https://www.youtube.com/watch?v=HyzD8pNlpwI
 *
 * > LCD
 * http://www.codeslinger.co.uk/pages/projects/gameboy/lcd.html
 */


static const int TOTAL_SCANLINE_CYCLES = 456;

static int scanline_cycles_left = TOTAL_SCANLINE_CYCLES;



/*---- LCD Control Status -----------------------------------------*/


static const int MODE2_SCANLINE_CYCLES = 376; // 456-80 (M2 lasts the first 80 cycles)
static const int MODE3_SCANLINE_CYCLES = 204; // 376-172 (M3 lasts 172 cycles after M2)
                                              // M0 lasts from cycles 204 to 0

static unsigned char lcdc_is_enabled() {

    return (*lcdc & 0x80); // true if bit 7 of lcd control is set
}

static void change_lcd_mode(unsigned char mode) {

    // first clear the mode to 0 (in case mode == 0x0) (set first two bits to 0)
    *lcdc_stat &= 0xFC; // 1111 1100

    *lcdc_stat |= mode;
}

static int lcdmode_interrupt_is_enabled(unsigned char mode) {

    /* LCD Interrupt can be triggered by mode changes,
     * As long as its respective mode interrupt is enabled
     *      LCDC Status Register:
     *          Bit 3: Mode 0 Interrupt Enabled
     *          Bit 4: Mode 1 Interrupt Enabled
     *          Bit 5: Mode 2 Interrupt Enabled
     */

    if (mode == 0 && (*lcdc_stat & 0x8))
        return 1;
    else if (mode == 1 && (*lcdc_stat & 0x10))
        return 1;
    else if (mode == 2 && (*lcdc_stat & 0x20))
        return 1;

    return 0;
}

static void set_lcd_stat() {

    /* LCD goes through 4 different modes, defined with bits 1 and 0
     *      (0) 00: H-Blank
     *      (1) 01: V-Blank
     *      (2) 10: Searching Sprites Atts
     *      (3) 11: Transfering Data to LCD Driver
     */
    
    if (!lcdc_is_enabled()) {
        
        scanline_cycles_left = TOTAL_SCANLINE_CYCLES; // scanline is anchored 

        *lcd_ly = 0; // current scanline is set to 0

        // set the LCD mode to V-Blank (1) during 'LCD Disabled'
        change_lcd_mode(0x1); 

    } else {
    
        /*
         * Check in which of the following modes we are:
         *      V-Blank mode
         *      H-Blank
         *      SSpritesAttr
         *      TransfDataToDriver
         *
         * If the LCD Stat mode needs to change, interrupt LCD
         *
         * If LCD Y Compare == LCD Y (current scanline):
         *      Set coincidence flag, else, clear it
         *      Call interrupt
         *
         * Update LCDC(ontrol) STAT
         */
    
        unsigned char current_mode = *lcdc_stat & 0x3;
        unsigned char mode = 0;

        if (*lcd_ly >= 144)
            mode = 1;
        else if (scanline_cycles_left >= MODE2_SCANLINE_CYCLES)
            mode = 2;
        else if (scanline_cycles_left >= MODE3_SCANLINE_CYCLES)
            mode = 3;

        if ((mode != current_mode) && lcdmode_interrupt_is_enabled(mode))
            request_interrupt(LCDSTAT_INTERRUPT);

        if (*lcd_ly == *lcd_lyc) {
            
            *lcdc_stat |= 0x4; // Set Flag: LCDC Stat bit 2 is a flag for LY == LYC

            // If LCD interrupts are enabled for LY==LYC
            // (This 'interrupt enabled' is a flag in bit 6 of LCDC Stat)
            if (*lcdc_stat & 0x40)
                request_interrupt(LCDSTAT_INTERRUPT);
            
        } else
            *lcdc_stat &= ~0x4; // Clear LCDC Stat Bit 2 Flag

        change_lcd_mode(mode);

    }


}



/*---- Main Logic and Execution -----------------------------------*/


static void draw_scanline() {

    //TODO
}

void ppu(int cycles) {

    set_lcd_stat();

    if (!lcdc_is_enabled())
        return;

    scanline_cycles_left -= cycles;

    /* If the scanline is completely drawn according to the time passed in cycles */
    if (scanline_cycles_left <= 0) {
        
        (*lcd_ly)++; /* Scanline advances, so we update the LCDC Y-Coordinate
                      * register, because it needs to hold the current scanline
                      */

        scanline_cycles_left = TOTAL_SCANLINE_CYCLES;

        unsigned char current_scanline = *lcd_ly;
        
        /* VBLANK is confirmed when LY >= 144, the resolution is 160x144,
         * but since the first scanline is 0, the scanline number 144 is actually the 145th.
         *
         * It means the cpu can use the VRAM without worrying, because it's not being used.
         * (Scanlines >= 144 and <= 153 are +invisible scanlines')
         */
        if (current_scanline == 144)
            request_interrupt(VBLANK_INTERRUPT); 

        else if (current_scanline > 153) /* if scanline goes above 153, reset to 0 */
            current_scanline = 0;

        else if (current_scanline < 144)
            draw_scanline();

    }
    

}
















void printvram() {

    unsigned short vram_start = 0x00a7;
    unsigned short vram_end = 0x0100;

    printf("printing memory %d\n", vram_start);

    for (int i=vram_start; i<vram_end; i++) {
        printf("$%02x ", memory[i]);
    }

    printf("\n");

}

void print_tiles() {


    unsigned short vram_start = 0x9800;
    unsigned short vram_end = 0x9BFF;

    /* printvram(); */

    char tiles[1000][8][8];

    int tilesn = 0;

    // for each tile (read 16 bytes)
    for (int i=vram_start; i<vram_end-16; i+=16) {

        int row = 0;

        // for each row in the tile (read 2 bytes)
        for (int j=i; j<i+8-2; j+=2) {

            unsigned char low_colorb = memory[j];
            unsigned char high_colorb = memory[j+1];

            int column = 0;

            // for each bit in the two bytes for the row
            for(int z=0; z<7; z++) {
                    
                unsigned char mask = 1 << (7-z);

                unsigned char mlc = low_colorb & mask;
                unsigned char mhc = high_colorb & mask;

                char color = ' ';

                if (mlc > 0 && mhc > 0)
                    color = '@';
                else if (mhc > 0 && mlc == 0)
                    color = 'x';
                else if (mhc == 0 && mlc > 0)
                    color = '.';

                tiles[tilesn][row][column++] = color;

            }

            row++;

        }

        tilesn++;
            
    }


    for (int row=0; row<32; row++) {

        for (int byter=0; byter<8; byter++) {

            for (int i=0; i<32; i++) {

                for (int bytec = 0; bytec<8; bytec++) {

                    printf("%c ", tiles[row+i][byter][bytec]);

                }


            }

            printf("\n");

        }


    }

}
