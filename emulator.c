#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

int time = 0;   /* time is in cycles */


int i=0; // debug i (counter for debugging)


/*
 *  Update is called 60 times per second
 *
 *  It keeps the CPU running in sync with the PPU,
 *  and draws a frame every time it's run
 *
 */
void update() {
    
    int cycles_run = 0;

    while (cycles_run < FRAME_MAX_CYCLES && i++ < 4263) {

        printf("Instruction Number: %d\n", i);
        cycles_run += cpu();

        // TODO: Receive inputs/do interrupts?
    }

    //TODO: Render

    time += cycles_run;
}


void emulate() {

    debug();
    while (registers.pc < 257 && i < 4263) {

        update();

        printf("current time: %d\n\n", time);
        sleep(1/60);    /* update runs 60 times per second */
    }

}


int main(int argc, char *argv[])
{    
    load_bootstrap_rom();

    boot();

    load_cartridge("tetris-jp.gb");

    emulate();

    /* print_tiles(); */

    return 0;
}



// IDEA: implement built in debugger
// IDEA: make smallest possible version of the emulator ?