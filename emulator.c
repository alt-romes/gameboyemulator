#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//#define DEBUGEMU
//#define DEBUGCPU
//#define DEBUGPPU

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

int emulation_time = 0;   /* time is in cycles */


int i=0; // debug i (counter for debugging)
int max_i = 10300000;

/*
 *  Update is called 60 times per second
 *
 *  It keeps the CPU running in sync with the PPU,
 *  and draws a frame every time it's run
 *
 */
void update() {
    
    int cycles_this_frame = 0;

    while (cycles_this_frame < FRAME_MAX_CYCLES && i++ < max_i) {

#ifdef DEBUGEMU
        printf("Instruction Number: %d\n", i);
#endif

        int cycles = cpu();

        ppu(cycles); /* The Pixel Processing Unit receives the 
                      * the amount of cycles run by the processor
                      * in order to keep it in sync with the processor.
                      */

        // TODO: Receive inputs/request interrupts?


        cycles_this_frame += cycles;
    }
#ifdef DEBUGEMU
    printf("RAN %d CYCLES", cycles_this_frame);
#endif

    render_frame();

    emulation_time += cycles_this_frame;
}


void emulate() {

    while (registers.pc < 257 && i < max_i) {

        update();

#ifdef DEBUGEMU
        printf("current time: %d\n\n", emulation_time);
#endif
        sleep(1/60);    /* update runs 60 times per second */
    }

}

static void boot() {

    init_gui();

}


int main(int argc, char *argv[])
{    
    load_bootstrap_rom();

    load_cartridge("zelda.gb");

    boot();

    emulate();

    return 0;
}

void run_tests() {

    // TODO and TO TRY
    load_tests();

    // http://slack.net/~ant/old/gb-tests/

    //    blarggs test - serial output
    if (memory[0xff02] == 0x81) {
        char c = memory[0xff01];
        printf("%c", c);
        memory[0xff02] = 0x0;
    }
}

// IDEA: implement built in debugger
// IDEA: make smallest possible version of the emulator ?
