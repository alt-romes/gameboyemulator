#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//#define DEBUGEMU
//#define DEBUGCPU
//#define DEBUGPPU

//#define USE_GRAPHICS

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

unsigned long debugger = 0;

unsigned int debugger_offset = 0;


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

#ifdef DEBUGEMU
        printf("Instruction Number: %d\n", registers.pc);
#endif




        int cycles = cpu();

        if (debugger && (!debugger_offset || !(debugger_offset--))) {

            char c = getchar();
            if (c == 'n')
                debugger_offset = 10;
            if (c == 'b')
                debugger_offset = 100;
            if (c == 'm')
                debugger_offset = 1000;

            if (debugger++ % 2) printf("\e[1;1H\e[2J\n");
        }


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

    while (emulation_time<11000000) {

        update();

#ifdef DEBUGEMU
        printf("current time: %lu\n\n", emulation_time);
#endif
        sleep(1/60);    /* update runs 60 times per second */
    }

}

static void boot() {

    init_gui();

}

void run_tests();

int main(int argc, char *argv[])
{    

    if (argc > 1 && argv[1][0]=='-' && argv[1][1]=='d') {

        debugger++;
        printf("\e[1;1H\e[2J\n");
    }



    run_tests();

    insert_cartridge("tetris-jp.gb");

    load_roms();

    boot();

    emulate();

    return 0;
}

void run_tests() {

    // TODO and TO TRY
    load_tests();

    boot_tests();

    emulate();

    // http://slack.net/~ant/old/gb-tests/

    //    blarggs test - serial output
    /* if (memory[0xff02] == 0x81) { */
    /*     char c = memory[0xff01]; */
    /*     printf("%c", c); */
    /*     memory[0xff02] = 0x0; */
    /* } */

    exit(0);
}

// IDEA: implement built in debugger
// IDEA: make smallest possible version of the emulator ?
