#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

unsigned long debugger = 0;

#include "memory.c"
#include "cpu.c"
#include "ppu.c"

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

            char c = getchar();
            if (c == 'n')
                debugger_offset = 10;
            if (c == 'b')
                debugger_offset = 100;
            if (c == 'm')
                debugger_offset = 1000;
            if (c == ',')
                debugger_offset = 5000;

        }


        ppu(cycles); /* The Pixel Processing Unit receives the
                      * the amount of cycles run by the processor
                      * in order to keep it in sync with the processor.
                      */

        // TODO: Receive inputs/request interrupts?

        cycles_this_frame += cycles;
    }

    // TODO: Track time spent in this frame, and sleep the rest until 1/60

    render_frame();

    emulation_time += cycles_this_frame;
}


void emulate() {

    while (1) {

        update();

        //sleep(1/60);    /* update runs 60 times per second */
    }

}

static void boot() {

    init_gui();

}

void run_tests();

int main(int argc, char *argv[])
{

    if (argc > 1 && argv[1][0]=='-' && argv[1][1]=='d') {

        if (argc > 2) debug_from = atoi(argv[2]);
        else debug_from = 0;

    }


    /* run_tests(); */

    insert_cartridge("tetris-jp.gb");

    load_roms();

    boot();

    emulate();

    return 0;
}

void run_tests() {

    // http://slack.net/~ant/old/gb-tests/

    load_tests();

    boot_tests();

    emulate();

    exit(0);
}

// IDEA: make smallest possible version of the emulator ?
