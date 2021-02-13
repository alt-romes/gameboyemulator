#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

unsigned long debugger = 0;

// I should really do proper header files i'm being too lazy
#include "memory.c"
#include "cpu.c"
#include "ppu.c"
#include "timer.c"

const int FRAME_MAX_CYCLES = 69905; /*
                                     *      Gameboy's CPU runs at 4.194304MHz
                                     *  which means it executes 4194304 cycles per second.
                                     *
                                     *      In each second we draw 60 frames
                                     *  which means every frame the cpu can run 4194304/60 ~ 69905 cycles
                                     *
                                     *      We can use this to sync graphics with procressing instructions
                                     */

unsigned long emulation_time = 0;   /* time is in cycles */

unsigned int debugger_offset = 0;
unsigned int debug_from = -1;

void process_input() {
    
    // Before applying new states, reset the previous ones to avoid bugs
    /* *joyp |= 0xF; */ // No longer needed bc no longer allow ANY write to the lower 4 bits

    // joypad_state is defined in memory after reading from $FF00 joypad reg
    // It holds information about the keys pressed
    // The upper 4 bits are for standard inputs, the lower 4 are for direction inputs

    /* printf("Before pressing, joypad is %x and joypad state is %x\n", *joyp, joypad_state); */
    // $FF00 bit 4 with value 0 selects direction keys
    if ((joypad_state & 0xF) && !(*joyp & 0x10)) {

        // Key pressed is a standard input and the type set
        // in the joypad register ($FF00) is standard

        *joyp &= ~(joypad_state & 0xF);

    }
    // $FF00 bit 5 with value 0 selects button keys
    else if ((joypad_state & 0xF0) && !(*joyp & 0x20)) {

        // Key pressed is a direction input and the type set
        // in the joypad register ($FF00) is direction

       *joyp &= ~(joypad_state >> 4);

    }
    else {

        // If no joypad_state is set, no key was inputted, so reset the key pressed in $ff00

        return;
    }

    request_interrupt(JOYPAD_INTERRUPT);

}

/*
 *  Update is called 60 times per second
 *
 *  It keeps the CPU running in sync with the PPU,
 *  and draws a frame every time it's run
 *
 */
void update() {

    unsigned int cycles_this_frame = 0;

    while (cycles_this_frame < FRAME_MAX_CYCLES) {

        if (registers.pc == debug_from)
            debugger++;

        int cycles = cpu();

        if (debugger && (!debugger_offset || !(debugger_offset--))) {
            int c = getchar();
            switch(c) {
            case 'n':
                debugger_offset = 10;\
                break;
            case 'b':
                debugger_offset = 100;
                break;
            case 'm':
                debugger_offset = 1000;
                break;
            case ',':
                debugger_offset = 5000;
                break;
            }
        }


        ppu(cycles); /* The Pixel Processing Unit receives the
                      * the amount of cycles run by the processor
                      * in order to keep it in sync with the processor.
                      */

        timer(cycles); /* Same thing as the PPU goes for timers */

        process_input();

        cycles_this_frame += cycles;
    }


    render_frame();

    emulation_time += cycles_this_frame;
}

static void boot() {

    // Set keys as "unpressed" when the nintendo starts
    *joyp |= 0xF;

    printf("Booting...\n");

}


void emulate() {

    boot();

    while (1) {

        clock_t clock_start = clock();

        update();

        clock_t clock_end = clock();
        double time_taken = ((double) (clock_end-clock_start))/CLOCKS_PER_SEC;

        sleep((1/60)-time_taken);
    }

}


int main(int argc, char *argv[]) {

    char* romstring = "tetris-jp.gb";

    char* testing = NULL;
    if (argc > 1 && argv[1][0]=='-' && argv[1][1]=='d') {

        if (argc > 2) debug_from = atoi(argv[2]);
        else debug_from = 0;
    }
    else if (argc > 1 && argv[1][0]=='-' && argv[1][1]=='t' && argc > 2) {

        testing = argv[2];

        if (argc > 3 && argv[3][0]=='-' && argv[3][1]=='d') {

            if (argc > 4) debug_from = atoi(argv[4]);
            else debug_from = 0x100;
        }
    }
    else if (argc > 1 && argv[1][0]=='-' && argv[1][1]=='r') {
        if (argc > 2)
            romstring = argv[2];
        else
            exit(3);
    }

    insert_cartridge(romstring);

    init_gui();

    load_roms();


    if (testing != NULL) {

        // TODO: To run the whole "cpu_instr" test, i need to implement MBC1
        // http://slack.net/~ant/old/gb-tests/
        load_tests(testing);

        boot_tests();
    }

    emulate();

    return 0;
}

// IDEA: make smallest possible version of the emulator ?
