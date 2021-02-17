
#include "timer.h"
#include "memory.h"
#include "cpu.h"

static const int TOTAL_DIVIDER_CYCLES = 256;

static int divider_cycles_left = TOTAL_DIVIDER_CYCLES;
static int counter_cycles_left = -1;

static int first_iteration = 1;

static unsigned char timer_is_enabled() {

    return (*tac & 4);
}

static void set_div() {

    if (divider_cycles_left > 0)
        return;

    (*tdiv)++;

    divider_cycles_left = TOTAL_DIVIDER_CYCLES;
}

static int get_counter_speed() {

    // (Cycles/Second) / (Interactions/Second) == Cycles/Interaction
    // The results from this calculation are hardcoded below

    /* 
     *  Frequency value of interactions
     *  00: 4096 Hz
     *  01: 262144 Hz
     *  10: 65536 Hz
     *  11: 16384 Hz
     */

    unsigned char val = *tac & 3; // get first 2 bits
    switch (val) {
        case 0:
            return 1024; 
        case 1:
            return 16;
        case 2:
            return 64;
        case 3:
            return 256;
    }
    return 0;
}

void timer(int cycles) {

    divider_cycles_left -= cycles;
    set_div();

    if ( !timer_is_enabled()  )
        return;

    if (first_iteration) {
        counter_cycles_left = get_counter_speed();
        first_iteration = 0;
    }

    // TODO: If call is the operation, cycles could be 24
    // If, z.b., counter_cycles_left = 4, tima would be set twice.
    counter_cycles_left -= cycles;

    if (counter_cycles_left <= 0) {

        counter_cycles_left += get_counter_speed();

        if (*tima == 255) {
            *tima = *tma;
            request_interrupt(TIMER_INTERRUPT);

        }
        else
            (*tima)++;
    }

}
