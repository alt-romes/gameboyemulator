static const int TOTAL_DIVIDER_CYCLES = 16384;

static int divider_cycles_left = TOTAL_DIVIDER_CYCLES;
static int counter_cycles_left = 0;

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

    unsigned char val = *tac & 3; // get first 2 bits
    switch (val) {
        case 0:
            return 4096;
        case 1:
            return 262144;
        case 2:
            return 65536;
        case 3:
            return 16384;
        default:
            return 0;
    }
}

void timer(int cycles) {

    divider_cycles_left -= cycles;
    set_div();

    if ( !timer_is_enabled()  )
        return;

    if (counter_cycles_left <= 0)
        counter_cycles_left = get_counter_speed();

    counter_cycles_left -= cycles;

    if (counter_cycles_left <= 0)
        (*tima)--;

    if (*tima <= 0) {
        *tima = *tma;
        request_interrupt(TIMER_INTERRUPT);
    }

}
