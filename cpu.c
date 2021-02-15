/*
 *  Gameboy Emulator: CPU
 *
 *
 *  Resources:
 *
 *  > Everything ~you wanted to know
 *  http://bgb.bircd.org/pandocs.htm
 *  > Pandocs active version
 *  https://gbdev.io/pandocs/
 *
 *  > Instruction Set + Registers
 *  https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
 *
 *  > More Opcodes
 *  http://gameboy.mongenel.com/dmg/opcodes.html
 *
 *  > Bootstrap ROM
 *  https://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM
 *
 *
 *  Notes:
 *
 *  Load memory.c before loading this file (cpu.c)
 *
 */



#include <assert.h>



/*---- Registers & Control ----------------------------------------*/


/*
 *  The Gameboy's CPU has 8 registers with size 8bits each
 *  They can be combined to form 16bit registers
 *
 *      16bit | hi | lo | name/function
 *      af    | a  | f  | accumulator & flags
 *      bc    | b  | c  |
 *      de    | d  | e  |
 *      hl    | h  | l  |
 *      sp    | -  | -  | stack pointer
 *      pc    | -  | -  | program counter
 *
 *  f is a special register called "flag register"
 *
 *      bit | name | explanation
 *      7   | z    |  zero flag
 *      6   | n    |  add/sub flag
 *      5   | h    |  half carry flag
 *      4   | cy   |  carry flag
 *      3-0 | -    |  unused (value=0)
 *
 */
struct registers {
    union {
        struct {
            unsigned char f;
            unsigned char a;
        };
        unsigned short af;
    };
    union {
        struct {
            unsigned char c;
            unsigned char b;
        };
        unsigned short bc;
    };
    union {
        struct {
            unsigned char e;
            unsigned char d;
        };
        unsigned short de;
    };
    union {
        struct {
            unsigned char l;
            unsigned char h;
        };
        unsigned short hl;
    };
    unsigned short sp;
    union {
        struct {
            unsigned char pclo;
            unsigned char pchi;
        };
        unsigned short pc;
    };
};

struct registers registers = {{{0}}};

static unsigned char interrupt_master_enable = 0;   /*Interrupt Master Enable Flag (enables or disables interrupts)*/

static unsigned char halted = 0;    /* If halted = 1, CPU is idle and waiting for interrupt request  */

static unsigned char stopped = 0;   /* If stopped = 1, CPU is stopped. */



/*---- Flags ------------------------------------------------------*/


#define FLAG_Z ((unsigned char) 128) // 1000 0000
#define FLAG_N ((unsigned char) 64) // 0100 0000
#define FLAG_H ((unsigned char) 32) // 0010 0000
#define FLAG_CY ((unsigned char) 16) // 0001 0000

static void set_flag(unsigned char flag) {

    registers.f |= flag;
}

static void clear_flag(unsigned char flag) {

    registers.f &= ~flag;
}





/*---- CPU Operations ---------------------------------------------*/

// Account for the extra cycles the JUMP instructions might take
static int extra_instruction_cycles = 0;

/*---- CPU Utils ----------------*/

static unsigned char read8bit_operand() {
    if (debugger)
        printf("8-bit read: %d\n", memory[registers.pc]);

    return memory[registers.pc++];
}

static char read8bit_signed_operand() {
    if (debugger)
        printf("signed 8-bit read: %d\n", (char) memory[registers.pc]);

    return (char) memory[registers.pc++];
}

static unsigned short read16bit_operand() {

    unsigned char operand_low = memory[registers.pc++];
    unsigned char operand_high = memory[registers.pc++];

    unsigned short operand = 0 | operand_high;
    operand = (operand << 8) | operand_low;

    if (debugger)
        printf("16-bit read: %d\n", operand);

    return operand;
}

static void debug() {
    printf("\n===============================\n");
    printf("register(A): %d\n", registers.a);
    printf("register(F): %d\n", registers.f);
    printf("register(B): %d\n", registers.b);
    printf("register(C): %d\n", registers.c);
    printf("register(D): %d\n", registers.d);
    printf("register(E): %d\n", registers.e);
    printf("register(H): %d\n", registers.h);
    printf("register(L): %d\n", registers.l);

    printf("===============================\n");
    printf("register(SP): %d\n", registers.sp);
    printf("register(PC): %d\n", registers.pc);

    printf("===============================\n");
    printf("register(AF): %d\n", registers.af);
    printf("register(BC): %d\n", registers.bc);
    printf("register(DE): %d\n", registers.de);
    printf("register(HL): %d\n", registers.hl);
    printf("===============================\n\n");
}



/*---- 8-Bit Loads --------------*/

static void load8bit(unsigned char * destination, unsigned char * source) {

    assert(!(source >= &memory[0] && source < &memory[0x10000-1]));
    assert(!(destination >= &memory[0] && destination < &memory[0x10000-1]));

    *destination = *source;

}

static void load8bit_operand(unsigned char * destination, void* _unused) {

    unsigned char operand = read8bit_operand();

    load8bit(destination, &operand);
}

static void load8bit_to_mem(unsigned short * reg_with_pointer, unsigned char * source) {


    extra_instruction_cycles = mmu_write8bit(*reg_with_pointer, *source); 
}

static void load8bit_to_mem_operand(unsigned char * source) {

    unsigned short address = read16bit_operand();
    load8bit_to_mem(&address, source);
}

static void load8bit_to_mem_from_operand(unsigned short * reg_with_pointer) {

    unsigned char value = read8bit_operand();
    load8bit_to_mem(reg_with_pointer, &value);

}

static void load8bit_from_mem(unsigned char* destination, unsigned short* reg_with_pointer) {

    mmu_read8bit(destination, *reg_with_pointer);
}

static void load8bit_from_mem_operand(unsigned char * destination) {

    unsigned short address = read16bit_operand();

    load8bit_from_mem(destination, &address);

}

static void load8bit_from_io_mem(unsigned char* destination, unsigned char* offset_reg) {

    unsigned short pos = 0xFF00 + *offset_reg;
    load8bit_from_mem(destination, &pos);
}

static void load8bit_from_io_mem_operand(unsigned char* destination) {

    unsigned char operand = read8bit_operand();
    unsigned short pos = 0xFF00 + operand;
    load8bit_from_mem(destination, &pos);
}

static void load8bit_to_io_mem(unsigned char* offset_reg, unsigned char* source) {

    unsigned short pos = 0xFF00 + *offset_reg;
    load8bit_to_mem(&pos, source);
}

static void load8bit_to_io_mem_operand(unsigned char* source) {

    unsigned char operand = read8bit_operand();
    unsigned short pos = 0xFF00 + operand;
    load8bit_to_mem(&pos, source);
}

static void load8bit_inc_to_mem() {

    load8bit_to_mem(&registers.hl, &registers.a);
    registers.hl++;
}

static void load8bit_inc_from_mem() {

    load8bit_from_mem(&registers.a, &registers.hl);
    registers.hl++;
}

static void load8bit_dec_to_mem() {

    load8bit_to_mem(&registers.hl, &registers.a);
    registers.hl--;
}

static void load8bit_dec_from_mem() {

    load8bit_from_mem(&registers.a, &registers.hl);
    registers.hl--;
}




/*---- 16-Bit Loads -------------*/

static void load16bit(unsigned short * destination, unsigned short *source) {

    *destination = *source;
}

static void load16bit_operand(unsigned short * destination) {

    unsigned short operand = read16bit_operand();

    load16bit(destination, &operand);
}

static void push_op(unsigned char * hi_reg, unsigned char * lo_reg) {

    memory[--registers.sp] = *hi_reg;
    memory[--registers.sp] = *lo_reg;
}

static void pop_op(unsigned char* hi_reg, unsigned char* lo_reg) {

    *lo_reg = memory[registers.sp++];
    *hi_reg = memory[registers.sp++];

    if (lo_reg == &registers.f)
        registers.f &= 0xf0;
}

static void load16bit_sp_operand_offset() {

    char operand = read8bit_signed_operand();
    unsigned short operand_plus_sp = operand + registers.sp;
    unsigned char lo = registers.sp & 0xFF;

    //half carry flag
    if ((lo & 0xF) + (operand & 0xF) > 0xF ) set_flag(FLAG_H);
    else clear_flag(FLAG_H);

    // carry flag
    if ((operand & 0xFF) + (lo & 0xFF) > 0xFF ) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);

    load16bit(&registers.hl, &operand_plus_sp);

    clear_flag(FLAG_Z | FLAG_N);
}

static void load16bit_sp_to_mem() {

    unsigned short operand = read16bit_operand();
    unsigned char lo = registers.sp & 0xFF;
    unsigned char hi = registers.sp >> 8;

    load8bit_to_mem(&operand, &lo);
    operand++;
    load8bit_to_mem(&operand, &hi);

}

/*---- 8-Bit ALU ----------------*/

static void add8bit(unsigned char* s) {

    // in C arithmetic is done with integers (int)

    // carry flag
    if ((registers.a & 0xFF) + (*s & 0xFF) > 0xFF) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);

    // half carry flag
    // (h is set if there's an overflow from the lowest 4 bits to the highest 4)
    if ((*s & 0xF) + (registers.a & 0xF) > 0xF) set_flag(FLAG_H);
    else clear_flag(FLAG_H);

    registers.a += *s;

    // zero flag
    if (registers.a == 0) set_flag(FLAG_Z);
    else clear_flag(FLAG_Z);

    // add/sub flag
    clear_flag(FLAG_N);

}

static void add8bit_from_mem() {

    add8bit(&memory[registers.hl]);
}

static void add8bit_operand() {

    unsigned char operand = read8bit_operand();
    add8bit(&operand);
}

static void sub(unsigned char* reg) {

    registers.a -= *reg;
    
    if ( (registers.a & 0xF) + (*reg & 0xF) > 0xF ) set_flag(FLAG_H); // Half carry if adding back the subtracted number changes the upper nibble
    else clear_flag(FLAG_H);

    if (registers.a) clear_flag(FLAG_Z);
    else set_flag(FLAG_Z);

    if ( ((registers.a + *reg) & 0xFF) < registers.a ) set_flag(FLAG_CY); // Carry if subtraction is bigger than original number
    else clear_flag(FLAG_CY);
    
    set_flag(FLAG_N);
}

static void sub_from_mem() {

    sub(&memory[registers.hl]);
}

static void sub_operand() {

    unsigned char operand = read8bit_operand();
    sub(&operand);
}

static void adc(unsigned char* s) {

    unsigned char adc = registers.f & FLAG_CY ? 1 : 0;

    // carry flag
    if (registers.a + *s + adc > 0xFF) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);
    
    // half carry flag
    // (h is set if there's an overflow from the lowest 4 bits to the highest 4)
    if ((*s & 0xF) + (registers.a & 0xF) + adc > 0xF) set_flag(FLAG_H);
    else clear_flag(FLAG_H);

    registers.a += *s + adc;

    // zero flag
    if (registers.a == 0) set_flag(FLAG_Z);
    else clear_flag(FLAG_Z);

    // add/sub flag
    clear_flag(FLAG_N);

}

static void sbc(unsigned char* reg) {

    unsigned char sbc = registers.f & FLAG_CY ? 1 : 0;

    registers.a -= *reg;
    registers.a -= sbc;
    
    if ( (registers.a & 0xF) + (*reg & 0xF) + sbc > 0xF ) set_flag(FLAG_H); // Half carry if adding back the subtracted number changes the upper nibble
    else clear_flag(FLAG_H);

    if (registers.a) clear_flag(FLAG_Z);
    else set_flag(FLAG_Z);

    if ( ((registers.a + *reg + sbc)) > 0xFF ) set_flag(FLAG_CY); // Carry if subtraction is bigger than original number
    else clear_flag(FLAG_CY);
    
    set_flag(FLAG_N);

}

static void sbc_from_mem() {

    sbc(&memory[registers.hl]);
}

static void sbc_operand() {

    unsigned char operand = read8bit_operand();
    sbc(&operand);
}

static void adc_from_mem(unsigned short* reg_with_pointer) {

    adc(&memory[*reg_with_pointer]);
}

static void adc_operand() {

    unsigned char operand = read8bit_operand();
    adc(&operand);
}

static void xor_reg(unsigned char* reg) {

    registers.a ^= *reg;

    if (registers.a == 0)
        set_flag(FLAG_Z);
    else
        clear_flag(FLAG_Z);

    clear_flag(FLAG_N | FLAG_H | FLAG_CY);
}

static void xor_reg_from_mem(unsigned short * reg_with_pointer) {

    xor_reg(&memory[*reg_with_pointer]);
}

static void xor_operand() {

    unsigned char operand = read8bit_operand();
    xor_reg(&operand);
}

static void and_reg(unsigned char* reg) {

    registers.a &= *reg;

    if (registers.a == 0)
        set_flag(FLAG_Z);
    else
        clear_flag(FLAG_Z);

    clear_flag(FLAG_N | FLAG_CY);
    set_flag(FLAG_H);
}

static void and_operand (){

    unsigned char operand = read8bit_operand();
    and_reg(&operand);

}

static void and_from_mem(){

    and_reg(&memory[registers.hl]);
}

static void or_reg(unsigned char* reg) {

    registers.a |= *reg;

    if (registers.a == 0)
        set_flag(FLAG_Z);
    else
        clear_flag(FLAG_Z);

    clear_flag(FLAG_N | FLAG_H | FLAG_CY);
}

static void or_from_mem(){

    or_reg(&memory[registers.hl]);
}

static void or_operand (){

    unsigned char operand = read8bit_operand();
    or_reg(&operand);

}

static void inc8bit(unsigned char* reg) {

    if ( (1 & 0xF) + (*reg & 0xF) > 0xF ) set_flag(FLAG_H);
    else clear_flag(FLAG_H);

    (*reg)++;

    if (*reg==0) set_flag(FLAG_Z);
    else clear_flag(FLAG_Z);

    clear_flag(FLAG_N);

}

static void inc8bit_from_mem() {

    inc8bit(&memory[registers.hl]);
}

static void dec8bit(unsigned char* reg) {

    (*reg)--;

    if ( ((*reg) & 0xF) + (1 & 0xF) > 0xF ) set_flag(FLAG_H); // If after decrementing, the addition overflows the lower nibble, then the subtraction had to borrow from the upper nibble 
    else clear_flag(FLAG_H);

    if (*reg==0) set_flag(FLAG_Z);
    else clear_flag(FLAG_Z);

    set_flag(FLAG_N);

}

static void dec8bit_from_mem(){

    dec8bit(&memory[registers.hl]);
}

static void cp_op(unsigned char* reg) {

    unsigned char s = *reg;
    unsigned char cp_a = registers.a - s;

    // zero flag
    if (cp_a == 0) set_flag(FLAG_Z);
    else clear_flag(FLAG_Z);

    // add/sub flag
    set_flag(FLAG_N);

    // half carry flag
    // (h is set if there's an overflow from the lowest 4 bits to the highest 4)
    // check if adding back the number subtracted alters the upper nibble
    if ( (cp_a & 0xF) + (s & 0xF) > 0xF ) set_flag(FLAG_H);
    else clear_flag(FLAG_H);

    // carry flag
    if (cp_a > registers.a) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);
}

static void cp_operand() {

    unsigned char s = read8bit_operand();
    cp_op(&s);
}

static void cp_mem(unsigned short* reg_w_pointer) {
    cp_op(&memory[*reg_w_pointer]);
}



/*---- 16-Bit Arithmetic --------*/

static void add16bit(unsigned short* source) {

    // Add 16 bits are two 8 bit adds
    // First, add low register L with low eight bits of source.
    // Then, add high register H with high eight bits of source, + carry from previous 

    if ( (registers.l & 0xF) + (*source & 0xF) > 0xF ) set_flag(FLAG_H);
    else clear_flag(FLAG_H);
    
    if ( registers.l + (*source & 0xFF) > 0xFF ) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);

    registers.l += *source & 0xFF;

    unsigned char carry = (registers.f & FLAG_CY ? 1 : 0);

    if ( (registers.h & 0xF) + ((*source >> 8) & 0xF) + carry > 0xF ) set_flag(FLAG_H);
    else clear_flag(FLAG_H);

    if ( registers.h + (*source >> 8) + carry > 0xFF ) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);

    registers.h = registers.h + (*source >> 8) + carry; // ADC to high byte

    clear_flag(FLAG_N);
}

static void inc16bit(unsigned short* reg) {

    (*reg)++;
}

static void dec16bit(unsigned short* reg) {

    (*reg)--;
}


static void add16bit_sp_operand() {


    char operand = read8bit_signed_operand();
    unsigned char lo = registers.sp & 0xFF;

    // TODO: i should have refactored the flags code when i had the chance to do it 

    //half carry flag
    if ((lo & 0xF) + (operand & 0xF) > 0xF ) set_flag(FLAG_H);
    else clear_flag(FLAG_H);

    // carry flag
    if ((operand & 0xFF) + (lo & 0xFF) > 0xFF ) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);

    registers.sp += operand;
    clear_flag(FLAG_Z | FLAG_N);

}



/*---- Miscellaneous ------------*/

static void nop () {

    // do nothing
}

static void disable_interrupts() {

    interrupt_master_enable = 0;
}

static void enable_interrupts() {

    interrupt_master_enable = 1;
}

static void halt() {

    halted = 1;
}

static void complement(){

    registers.a = ~registers.a;
    set_flag(FLAG_N | FLAG_H);
}

static void swap (unsigned char* reg) {

    unsigned char new_lo = *reg >> 4;
    unsigned char new_hi = *reg << 4;
    *reg = (((0x00) | new_lo) | new_hi);

    if (*reg)
        clear_flag(FLAG_Z);
    else
        set_flag(FLAG_Z);

    clear_flag(FLAG_N | FLAG_H | FLAG_CY);
}

static void stop_cpu() {

    stopped = 1;

    printf("~Stopped.\n");
}

static void ccf_op() {

    unsigned char flag_cy_value = registers.f & FLAG_CY;

    if(flag_cy_value)
        clear_flag(FLAG_CY);
    else
        set_flag(FLAG_CY);

    clear_flag(FLAG_N | FLAG_H);
}

static void scf_op() {

    set_flag(FLAG_CY);

    clear_flag(FLAG_N | FLAG_H);
}


/*---- Rotates & Shifts ---------*/

static void rl_op(unsigned char* reg) {

    // check if carry is set
    unsigned char hadcarry = (registers.f & FLAG_CY) >> 4;

    // check highest bit from reg to check if carry should be set
    if ((*reg >> 7) & 1) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);

    // rotate and add carry
    *reg <<= 1;
    *reg |= hadcarry;

    if (*reg == 0) set_flag(FLAG_Z);
    else clear_flag(FLAG_Z);

    clear_flag(FLAG_N | FLAG_H);
}

static void rla_op() {

    rl_op(&registers.a);
    clear_flag(FLAG_Z);
}

static void sla_op(unsigned char* reg) {
    unsigned char left_most_bit = 0x80 & *reg;
    *reg = *reg<<1;

    if (left_most_bit)
        set_flag(FLAG_CY);
    else
        clear_flag(FLAG_CY);

    if (!*reg)
        set_flag(FLAG_Z);
    else
        clear_flag(FLAG_Z);

    clear_flag(FLAG_N | FLAG_H);
}

static void srl_op(unsigned char* reg) {
    unsigned char right_most_bit = 0x1 & *reg;
    *reg = *reg>>1;

    if (right_most_bit)
        set_flag(FLAG_CY);
    else
        clear_flag(FLAG_CY);

    if (!*reg)
        set_flag(FLAG_Z);
    else
        clear_flag(FLAG_Z);

    clear_flag(FLAG_N | FLAG_H);
}

static void rr_op(unsigned char* reg) {
    unsigned char right_most_bit = 0x1 & *reg;
    unsigned char carry_flag_value = FLAG_CY & registers.f;
    *reg = *reg>>1;

    if(carry_flag_value)
        *reg = *reg | 0x80;
    else
        *reg = *reg & 0x7F;

    if (right_most_bit)
        set_flag(FLAG_CY);
    else
        clear_flag(FLAG_CY);

    if (!*reg)
        set_flag(FLAG_Z);
    else
        clear_flag(FLAG_Z);

    clear_flag(FLAG_N | FLAG_H);
}

static void rra_op() {

    rr_op(&registers.a);

    clear_flag(FLAG_Z | FLAG_N | FLAG_H);
}

static void rlc_op(unsigned char* reg) {
    unsigned char left_most_bit = 0x80 & *reg;
    *reg = *reg<<1;

    if (left_most_bit) {
        *reg = *reg | 0x1;
        set_flag(FLAG_CY);
    }else{
        *reg = *reg & 0xFE;
        clear_flag(FLAG_CY);
    }

    if (!*reg)
        set_flag(FLAG_Z);
    else
        clear_flag(FLAG_Z);

    clear_flag(FLAG_H | FLAG_N);
}

static void rlca_op() {

    rlc_op(&registers.a);

    clear_flag(FLAG_Z);
}

static void rrc_op(unsigned char* reg) {
    unsigned char right_most_bit = 0x1 & *reg;
    *reg = *reg>>1;

    if(right_most_bit){
        *reg = *reg | 0x80;
        set_flag(FLAG_CY);
    }else{
        *reg = *reg & 0x7F;
        clear_flag(FLAG_CY);
    }

    if(!*reg)
        set_flag(FLAG_Z);
    else
        clear_flag(FLAG_Z);

    clear_flag(FLAG_H | FLAG_N);
}

static void rrca_op() {

    rrc_op(&registers.a);

    clear_flag(FLAG_Z);
}

static void sra_op(unsigned char* reg) {

    unsigned char left_most_bit = 0x80 & *reg;
    unsigned char right_most_bit = 0x1 & *reg;
    *reg = *reg>>1;

    if (right_most_bit)
        set_flag(FLAG_CY);
    else
        clear_flag(FLAG_CY);

    if (left_most_bit)
        *reg = *reg | 0x80;
    else
        *reg = *reg & 0x7F;

    if (!*reg)
        set_flag(FLAG_Z);
    else
        clear_flag(FLAG_Z);

    clear_flag(FLAG_H | FLAG_N);
}

static void rlc_from_mem() {

    rlc_op(&memory[registers.hl]);
}

static void rrc_from_mem() {

    rrc_op(&memory[registers.hl]);
}

static void rl_from_mem() {

    rl_op(&memory[registers.hl]);
}

static void rr_from_mem() {

    rr_op(&memory[registers.hl]);
}

static void sla_from_mem() {

    sla_op(&memory[registers.hl]);
}

static void sra_from_mem() {

    sra_op(&memory[registers.hl]);
}

static void swap_from_mem() {

    swap(&memory[registers.hl]);
}

static void srl_from_mem() {

    srl_op(&memory[registers.hl]);
}

static void daa_op(){
    unsigned char correction = 0;
    unsigned char flag_cy_value = FLAG_CY & registers.f;
    unsigned char flag_h_value = FLAG_H & registers.f;
    unsigned char flag_n_value = FLAG_N & registers.f;

    if (flag_h_value || (!flag_n_value && (registers.a & 0xF) > 9))
        correction |= 0x6;

    if (flag_cy_value || (!flag_n_value && registers.a > 0x99)) {
        
        correction |= 0x60;
        set_flag(FLAG_CY);
    }
    else
        clear_flag(FLAG_CY);

    registers.a += flag_n_value ? -correction : correction;

    if (registers.a)
        clear_flag(FLAG_Z);
    else
        set_flag(FLAG_Z);

    clear_flag(FLAG_H);

}

/*---- Bit Opcodes --------------*/

// Tests bit of a register
static void bit_op(void* n, unsigned char * reg) {

    unsigned char n_byte = (unsigned char) n;

    if ( (*reg >> n_byte ) & 1 ) clear_flag(FLAG_Z);
    else set_flag(FLAG_Z);

    clear_flag(FLAG_N);
    set_flag(FLAG_H);
}

static void bit_op_from_mem(void* n, unsigned short * reg) {

    bit_op(n, &memory[*reg]);
}

static void set_op(void* n, unsigned char* reg) {

    unsigned char n_byte = (unsigned char) n;

    *reg |= (1 << (n_byte));
}

static void set_op_from_mem(void* n , unsigned short* reg_with_pointer) {

    set_op(n, &memory[*reg_with_pointer]);
}

static void res_op(void* n, unsigned char* reg) {

    unsigned char n_byte = (unsigned char) n;

    *reg &= ~(1 << (n_byte));
}

static void res_from_mem(void* n , unsigned short* reg_with_pointer) {

    res_op(n, &memory[*reg_with_pointer]);
}

/*---- Jumps --------------------*/

static void jump(unsigned short *reg) {

    registers.pc = *reg;
    extra_instruction_cycles = 4;
}

static void jump_operand(void* unused , void* unused2) {

    registers.pc = read16bit_operand();
    extra_instruction_cycles = 4;
}

static void jump_add_operand() {

    registers.pc += read8bit_signed_operand();
    extra_instruction_cycles = 4;
}

//TODO: general jump condition?

// Sets program counter to operand on condition
// Jump_cond is 0 if should jump if flag == 0, and is a value > 0 if should jump if flag is not zero
static void jump_condition_operand(void* flag, void* jump_cond) {

    unsigned char flag_value = (unsigned char) flag & registers.f ? 1 : 0;

    unsigned short operand = read16bit_operand();

    if (flag_value == (unsigned char) jump_cond) {

        registers.pc = operand;
        extra_instruction_cycles = 4;
    }

}

// Adds operand to the current program counter on condition
// jump_cond is 0 if condition is NOT FLAG, jump_cond is 1 if condition is FLAG
static void jump_condition_add_operand(void* flag, void* jump_cond) {

    unsigned char flag_value = (unsigned char) flag & registers.f ? 1 : 0;

    char operand = read8bit_signed_operand();

    if (flag_value == (unsigned char) jump_cond) {

        registers.pc += operand;
        extra_instruction_cycles = 4;
    }
}




/*---- Calls --------------------*/

static void call(unsigned short address) {

    memory[--registers.sp] = registers.pchi;
    memory[--registers.sp] = registers.pclo;

    registers.pc = address;

}

static void call_operand() {

    unsigned short operand = read16bit_operand();

    call(operand);
}

static void rst(void* address){

    call((unsigned short) address);

}

// call_cond is 0 if condition is NOT FLAG, call_cond is 1 if condition is FLAG
// ex: call nz 123

static void call_condition(void* flag, void* call_cond) {

    unsigned char flag_value = (unsigned char) flag & registers.f ? 1 : 0;
    unsigned short operand = read16bit_operand();

    if (flag_value == (unsigned char) call_cond) {

        call(operand);
        extra_instruction_cycles = 12;
    }
}

/*---- Returns ------------------*/

static void ret_op() {

    registers.pclo = memory[registers.sp++];
    registers.pchi = memory[registers.sp++];
}

// jump_cond is 0 if condition is NOT FLAG, jump_cond is 1 if condition is FLAG
static void ret_condition(void* flag, void* ret_cond) {

    unsigned char flag_value = (unsigned char) flag & registers.f ? 1 : 0;

    if (flag_value == (unsigned char) ret_cond) {

        ret_op();
        extra_instruction_cycles = 12;
    }

}

static void ret_interrupt(){
    ret_op();
    enable_interrupts();
}





/*---- Instructions -----------------------------------------------*/


/*
 *  Instruction set with disassembly, callback and parameters
 */
struct instruction {
    char* disassembly;
    void (*execute)();
    void* exec_argv1;
    void* exec_argv2;
};


/*
 * Instruction disassemblies copied from https://github.com/CTurt/Cinoop
 */
const struct instruction instructions[256] = {
    { "NOP", nop},                          // 0x00
    { "LD BC, 0x%04X", load16bit_operand, &registers.bc},                // 0x01
    { "LD (BC), A", load8bit_to_mem, &registers.bc, &registers.a },                   // 0x02
    { "INC BC", inc16bit, &registers.bc},                       // 0x03
    { "INC B", inc8bit, &registers.b},                        // 0x04
    { "DEC B", dec8bit, &registers.b},                        // 0x05
    { "LD B, 0x%02X", load8bit_operand, &registers.b },                 // 0x06
    { "RLCA", rlca_op},                         // 0x07
    { "LD (0x%04X), SP", load16bit_sp_to_mem},              // 0x08
    { "ADD HL, BC", add16bit, &registers.bc},                   // 0x09
    { "LD A, (BC)", load8bit_from_mem, &registers.a, &registers.bc},                   // 0x0a
    { "DEC BC", dec16bit, &registers.bc},                       // 0x0b
    { "INC C", inc8bit, &registers.c },                        // 0x0c
    { "DEC C", dec8bit, &registers.c},                        // 0x0d
    { "LD C, 0x%02X", load8bit_operand, &registers.c },                 // 0x0e
    { "RRCA", rrca_op},                         // 0x0f
    { "STOP", stop_cpu},                         // 0x10
    { "LD DE, 0x%04X", load16bit_operand, &registers.de },                // 0x11
    { "LD (DE), A", load8bit_to_mem, &registers.de, &registers.a},                   // 0x12
    { "INC DE", inc16bit, &registers.de},                       // 0x13
    { "INC D", inc8bit, &registers.d},                        // 0x14
    { "DEC D", dec8bit, &registers.d},                        // 0x15
    { "LD D, 0x%02X", load8bit_operand, &registers.d},                 // 0x16
    { "RLA", rla_op},                          // 0x17
    { "JR 0x%02X", jump_add_operand},                    // 0x18
    { "ADD HL, DE", add16bit, &registers.de},                   // 0x19
    { "LD A, (DE)", load8bit_from_mem, &registers.a, &registers.de },                   // 0x1a
    { "DEC DE", dec16bit, &registers.de},                       // 0x1b
    { "INC E", inc8bit, &registers.e},                        // 0x1c
    { "DEC E", dec8bit, &registers.e},                        // 0x1d
    { "LD E, 0x%02X", load8bit_operand, &registers.e},                 // 0x1e
    { "RRA", rra_op},                          // 0x1f
    { "JR NZ, 0x%02X", jump_condition_add_operand, (void*) FLAG_Z, (void*) 0 },                // 0x20
    { "LD HL, 0x%04X", load16bit_operand, &registers.hl },                // 0x21
    { "LDI (HL), A", load8bit_inc_to_mem},                  // 0x22
    { "INC HL", inc16bit, &registers.hl},                       // 0x23
    { "INC H", inc8bit, &registers.h},                        // 0x24
    { "DEC H", dec8bit, &registers.h},                        // 0x25
    { "LD H, 0x%02X", load8bit_operand, &registers.h},                 // 0x26
    { "DAA", daa_op},                          // 0x27
    { "JR Z, 0x%02X", jump_condition_add_operand, (void*) FLAG_Z, (void*) 1},                 // 0x28
    { "ADD HL, HL", add16bit, &registers.hl},                   // 0x29
    { "LDI A, (HL)", load8bit_inc_from_mem},                  // 0x2a
    { "DEC HL", dec16bit, &registers.hl},                       // 0x2b
    { "INC L", inc8bit, &registers.l},                        // 0x2c
    { "DEC L", dec8bit, &registers.l},                        // 0x2d
    { "LD L, 0x%02X", load8bit_operand, &registers.l},                 // 0x2e
    { "CPL", complement},                          // 0x2f
    { "JR NC, 0x%02X", jump_condition_add_operand, (void*) FLAG_CY, (void*) 0},                // 0x30
    { "LD SP, 0x%04X", load16bit_operand, &registers.sp},             // 0x31
    { "LDD (HL), A", load8bit_dec_to_mem},                  // 0x32
    { "INC SP", inc16bit, &registers.sp},                       // 0x33
    { "INC (HL)", inc8bit_from_mem},                     // 0x34
    { "DEC (HL)", dec8bit_from_mem},                     // 0x35
    { "LD (HL), 0x%02X", load8bit_to_mem_from_operand, &registers.hl},              // 0x36
    { "SCF", scf_op},                          // 0x37
    { "JR C, 0x%02X", jump_condition_add_operand, (void*) FLAG_CY, (void*) 1},                 // 0x38
    { "ADD HL, SP", add16bit, &registers.sp},                   // 0x39
    { "LDD A, (HL)", load8bit_dec_from_mem},                  // 0x3a
    { "DEC SP", dec16bit, &registers.sp},                       // 0x3b
    { "INC A", inc8bit, &registers.a},                        // 0x3c
    { "DEC A", dec8bit, &registers.a},                        // 0x3d
    { "LD A, 0x%02X", load8bit_operand, &registers.a},                 // 0x3e
    { "CCF", ccf_op},                          // 0x3f
    { "LD B, B", load8bit, &registers.b, &registers.b},                      // 0x40
    { "LD B, C", load8bit, &registers.b, &registers.c},                      // 0x41
    { "LD B, D", load8bit, &registers.b, &registers.d},                      // 0x42
    { "LD B, E", load8bit, &registers.b, &registers.e},                      // 0x43
    { "LD B, H", load8bit, &registers.b, &registers.h},                      // 0x44
    { "LD B, L", load8bit, &registers.b, &registers.l},                      // 0x45
    { "LD B, (HL)", load8bit_from_mem, &registers.b, &registers.hl},                   // 0x46
    { "LD B, A", load8bit, &registers.b, &registers.a},                      // 0x47
    { "LD C, B", load8bit, &registers.c, &registers.b},                      // 0x48
    { "LD C, C", load8bit, &registers.c, &registers.c},                      // 0x49
    { "LD C, D", load8bit, &registers.c, &registers.d},                      // 0x4a
    { "LD C, E", load8bit, &registers.c, &registers.e},                      // 0x4b
    { "LD C, H", load8bit, &registers.c, &registers.h},                      // 0x4c
    { "LD C, L", load8bit, &registers.c, &registers.l},                      // 0x4d
    { "LD C, (HL)", load8bit_from_mem, &registers.c, &registers.hl},                   // 0x4e
    { "LD C, A", load8bit, &registers.c, &registers.a},                      // 0x4f
    { "LD D, B", load8bit, &registers.d, &registers.b},                      // 0x50
    { "LD D, C", load8bit, &registers.d, &registers.c},                      // 0x51
    { "LD D, D", load8bit, &registers.d, &registers.d},                      // 0x52
    { "LD D, E", load8bit, &registers.d, &registers.e},                      // 0x53
    { "LD D, H", load8bit, &registers.d, &registers.h},                      // 0x54
    { "LD D, L", load8bit, &registers.d, &registers.l},                      // 0x55
    { "LD D, (HL)", load8bit_from_mem, &registers.d, &registers.hl},                   // 0x56
    { "LD D, A", load8bit, &registers.d, &registers.a},                      // 0x57
    { "LD E, B", load8bit, &registers.e, &registers.b},                      // 0x58
    { "LD E, C", load8bit, &registers.e, &registers.c},                      // 0x59
    { "LD E, D", load8bit, &registers.e, &registers.d},                      // 0x5a
    { "LD E, E", load8bit, &registers.e, &registers.e},                      // 0x5b
    { "LD E, H", load8bit, &registers.e, &registers.h},                      // 0x5c
    { "LD E, L", load8bit, &registers.e, &registers.l},                      // 0x5d
    { "LD E, (HL)", load8bit_from_mem, &registers.e, &registers.hl},                   // 0x5e
    { "LD E, A", load8bit, &registers.e, &registers.a},                      // 0x5f
    { "LD H, B", load8bit, &registers.h, &registers.b},                      // 0x60
    { "LD H, C", load8bit, &registers.h, &registers.c},                      // 0x61
    { "LD H, D", load8bit, &registers.h, &registers.d},                      // 0x62
    { "LD H, E", load8bit, &registers.h, &registers.e},                      // 0x63
    { "LD H, H", load8bit, &registers.h, &registers.h},                      // 0x64
    { "LD H, L", load8bit, &registers.h, &registers.l},                      // 0x65
    { "LD H, (HL)", load8bit_from_mem, &registers.h, &registers.hl},                   // 0x66
    { "LD H, A", load8bit, &registers.h, &registers.a},                      // 0x67
    { "LD L, B", load8bit, &registers.l, &registers.b},                      // 0x68
    { "LD L, C", load8bit, &registers.l, &registers.c},                      // 0x69
    { "LD L, D", load8bit, &registers.l, &registers.d},                      // 0x6a
    { "LD L, E", load8bit, &registers.l, &registers.e},                      // 0x6b
    { "LD L, H", load8bit, &registers.l, &registers.h},                      // 0x6c
    { "LD L, L", load8bit, &registers.l, &registers.l},                      // 0x6d
    { "LD L, (HL)", load8bit_from_mem, &registers.l, &registers.hl},                   // 0x6e
    { "LD L, A", load8bit, &registers.l, &registers.a},                      // 0x6f
    { "LD (HL), B", load8bit_to_mem, &registers.hl, &registers.b},                   // 0x70
    { "LD (HL), C", load8bit_to_mem, &registers.hl, &registers.c},                   // 0x71
    { "LD (HL), D", load8bit_to_mem, &registers.hl, &registers.d},                   // 0x72
    { "LD (HL), E", load8bit_to_mem, &registers.hl, &registers.e},                   // 0x73
    { "LD (HL), H", load8bit_to_mem, &registers.hl, &registers.h},                   // 0x74
    { "LD (HL), L", load8bit_to_mem, &registers.hl, &registers.l},                   // 0x75
    { "HALT", halt},                         // 0x76
    { "LD (HL), A", load8bit_to_mem, &registers.hl, &registers.a },                   // 0x77
    { "LD A, B", load8bit, &registers.a, &registers.b},                      // 0x78
    { "LD A, C", load8bit, &registers.a, &registers.c},                      // 0x79
    { "LD A, D", load8bit, &registers.a, &registers.d},                      // 0x7a
    { "LD A, E", load8bit, &registers.a, &registers.e},                      // 0x7b
    { "LD A, H", load8bit, &registers.a, &registers.h},                      // 0x7c
    { "LD A, L", load8bit, &registers.a, &registers.l},                      // 0x7d
    { "LD A, (HL)", load8bit_from_mem, &registers.a, &registers.hl},                   // 0x7e
    { "LD A, A", load8bit, &registers.a, &registers.a},                      // 0x7f
    { "ADD A, B", add8bit, &registers.b},                     // 0x80
    { "ADD A, C", add8bit, &registers.c},                     // 0x81
    { "ADD A, D", add8bit, &registers.d},                     // 0x82
    { "ADD A, E", add8bit, &registers.e},                     // 0x83
    { "ADD A, H", add8bit, &registers.h},                     // 0x84
    { "ADD A, L", add8bit, &registers.l},                     // 0x85
    { "ADD A, (HL)", add8bit_from_mem},                  // 0x86
    { "ADD A", add8bit, &registers.a},                        // 0x87
    { "ADC B", adc, &registers.b},                        // 0x88
    { "ADC C", adc, &registers.c},                        // 0x89
    { "ADC D", adc, &registers.d},                        // 0x8a
    { "ADC E", adc, &registers.e},                        // 0x8b
    { "ADC H", adc, &registers.h},                        // 0x8c
    { "ADC L", adc, &registers.l},                        // 0x8d
    { "ADC (HL)", adc_from_mem, &registers.hl},                     // 0x8e
    { "ADC A", adc, &registers.a},                        // 0x8f
    { "SUB B", sub, &registers.b},                        // 0x90
    { "SUB C", sub, &registers.c},                        // 0x91
    { "SUB D", sub, &registers.d},                        // 0x92
    { "SUB E", sub, &registers.e},                        // 0x93
    { "SUB H", sub, &registers.h},                        // 0x94
    { "SUB L", sub, &registers.l},                        // 0x95
    { "SUB (HL)", sub_from_mem},                     // 0x96
    { "SUB A", sub, &registers.a},                        // 0x97
    { "SBC B", sbc, &registers.b},                        // 0x98
    { "SBC C", sbc, &registers.c},                        // 0x99
    { "SBC D", sbc, &registers.d},                        // 0x9a
    { "SBC E", sbc, &registers.e},                        // 0x9b
    { "SBC H", sbc, &registers.h},                        // 0x9c
    { "SBC L", sbc, &registers.l},                        // 0x9d
    { "SBC (HL)", sbc_from_mem},                     // 0x9e
    { "SBC A", sbc, &registers.a},                        // 0x9f
    { "AND B", and_reg, &registers.b},                        // 0xa0
    { "AND C", and_reg, &registers.c},                        // 0xa1
    { "AND D", and_reg, &registers.d},                        // 0xa2
    { "AND E", and_reg, &registers.e},                        // 0xa3
    { "AND H", and_reg, &registers.h},                        // 0xa4
    { "AND L", and_reg, &registers.l},                        // 0xa5
    { "AND (HL)", and_from_mem},                     // 0xa6
    { "AND A", and_reg, &registers.a},                        // 0xa7
    { "XOR B", xor_reg, &registers.b},                        // 0xa8
    { "XOR C", xor_reg, &registers.c},                        // 0xa9
    { "XOR D", xor_reg, &registers.d},                        // 0xaa
    { "XOR E", xor_reg, &registers.e},                        // 0xab
    { "XOR H", xor_reg, &registers.h},                        // 0xac
    { "XOR L", xor_reg, &registers.l},                        // 0xad
    { "XOR (HL)", xor_reg_from_mem, &registers.hl},                     // 0xae
    { "XOR A", xor_reg, &registers.a},                        // 0xaf
    { "OR B", or_reg, &registers.b},                         // 0xb0
    { "OR C", or_reg, &registers.c},                         // 0xb1
    { "OR D", or_reg, &registers.d},                         // 0xb2
    { "OR E", or_reg, &registers.e},                         // 0xb3
    { "OR H", or_reg, &registers.h},                         // 0xb4
    { "OR L", or_reg, &registers.l},                         // 0xb5
    { "OR (HL)", or_from_mem},                      // 0xb6
    { "OR A", or_reg, &registers.a},                         // 0xb7
    { "CP B", cp_op, &registers.b},                         // 0xb8
    { "CP C", cp_op, &registers.c},                         // 0xb9
    { "CP D", cp_op, &registers.d},                         // 0xba
    { "CP E", cp_op, &registers.e},                         // 0xbb
    { "CP H", cp_op, &registers.h},                         // 0xbc
    { "CP L", cp_op, &registers.l},                         // 0xbd
    { "CP (HL)", cp_mem, &registers.hl},                      // 0xbe
    { "CP A", cp_op, &registers.a},                         // 0xbf
    { "RET NZ", ret_condition, (void*) FLAG_Z, (void*) 0},                       // 0xc0
    { "POP BC", pop_op, &registers.b, &registers.c},                       // 0xc1
    { "JP NZ, 0x%04X", jump_condition_operand, (void*) FLAG_Z, (void*) 0},                // 0xc2
    { "JP 0x%04X", jump_operand},                    // 0xc3
    { "CALL NZ, 0x%04X", call_condition, (void*) FLAG_Z, (void*) 0},              // 0xc4
    { "PUSH BC", push_op, &registers.b, &registers.c },                      // 0xc5
    { "ADD A, 0x%02X", add8bit_operand},                // 0xc6
    { "RST 0x00", rst, (void*) 0x00},                     // 0xc7
    { "RET Z", ret_condition, (void *) FLAG_Z, (void*) 1},                        // 0xc8
    { "RET", ret_op},                          // 0xc9
    { "JP Z, 0x%04X", jump_condition_operand, (void*) FLAG_Z, (void*) 1},                 // 0xca
    { "CB %02X", NULL}, //should never fall here                     // 0xcb
    { "CALL Z, 0x%04X", call_condition, (void*) FLAG_Z, (void*) 1},               // 0xcc
    { "CALL 0x%04X", call_operand},                  // 0xcd
    { "ADC 0x%02X", adc_operand},                   // 0xce
    { "RST 0x08",  rst, (void*) 0x08},                     // 0xcf
    { "RET NC", ret_condition, (void *) FLAG_CY, (void*) 0},                       // 0xd0
    { "POP DE", pop_op, &registers.d, &registers.e},                       // 0xd1
    { "JP NC, 0x%04X", jump_condition_operand, (void*) FLAG_CY, (void*) 0},                // 0xd2
    { "UNKNOWN", NULL},                      // 0xd3
    { "CALL NC, 0x%04X", call_condition, (void*) FLAG_CY, (void*) 0},              // 0xd4
    { "PUSH DE", push_op, &registers.d, &registers.e},                      // 0xd5
    { "SUB 0x%02X", sub_operand},                   // 0xd6
    { "RST 0x10",  rst, (void*) 0x10},                     // 0xd7
    { "RET C", ret_condition, (void *) FLAG_CY, (void*) 1},                        // 0xd8
    { "RETI", ret_interrupt},                         // 0xd9
    { "JP C, 0x%04X", jump_condition_operand, (void*) FLAG_CY, (void*) 1},                 // 0xda
    { "UNKNOWN", NULL},                      // 0xdb
    { "CALL C, 0x%04X", call_condition, (void*) FLAG_CY, (void*) 1},               // 0xdc
    { "UNKNOWN", NULL},                      // 0xdd
    { "SBC 0x%02X", sbc_operand},                   // 0xde
    { "RST 0x18",  rst, (void*) 0x18},                     // 0xdf
    { "LD (0xFF00 + 0x%02X), A", load8bit_to_io_mem_operand, &registers.a},      // 0xe0
    { "POP HL", pop_op, &registers.h, &registers.l},                       // 0xe1
    { "LD (0xFF00 + C), A", load8bit_to_io_mem, &registers.c, &registers.a},           // 0xe2
    { "UNKNOWN", NULL},                      // 0xe3
    { "UNKNOWN", NULL},                      // 0xe4
    { "PUSH HL", push_op, &registers.h, &registers.l},                      // 0xe5
    { "AND 0x%02X", and_operand},                   // 0xe6
    { "RST 0x20",  rst, (void*) 0x20},                     // 0xe7
    { "ADD SP,0x%02X", add16bit_sp_operand},                // 0xe8
    { "JP HL", jump, &registers.hl},                        // 0xe9
    { "LD (0x%04X), A", load8bit_to_mem_operand, &registers.a},               // 0xea
    { "UNKNOWN", NULL},                      // 0xeb
    { "UNKNOWN", NULL},                      // 0xec
    { "UNKNOWN", NULL},                      // 0xed
    { "XOR 0x%02X", xor_operand},                   // 0xee
    { "RST 0x28",  rst, (void*) 0x28},                     // 0xef
    { "LD A, (0xFF00 + 0x%02X)", load8bit_from_io_mem_operand, &registers.a},      // 0xf0
    { "POP AF", pop_op, &registers.a, &registers.f},                       // 0xf1
    { "LD A, (0xFF00 + C)", load8bit_from_io_mem, &registers.a, &registers.c},           // 0xf2
    { "DI", disable_interrupts},                           // 0xf3
    { "UNKNOWN", NULL},                      // 0xf4
    { "PUSH AF", push_op, &registers.a, &registers.f},                      // 0xf5
    { "OR 0x%02X", or_operand},                    // 0xf6
    { "RST 0x30",  rst, (void*) 0x30},                     // 0xf7
    { "LD HL, SP+0x%02X", load16bit_sp_operand_offset},             // 0xf8
    { "LD SP, HL", load16bit, &registers.sp, &registers.hl},                    // 0xf9
    { "LD A, (0x%04X)", load8bit_from_mem_operand, &registers.a},               // 0xfa
    { "EI", enable_interrupts},                           // 0xfb
    { "UNKNOWN", NULL},                      // 0xfc
    { "UNKNOWN", NULL},                      // 0xfd
    { "CP 0x%02X", cp_operand},                    // 0xfe
    { "RST 0x38",  rst, (void*) 0x38},                     // 0xff
};

// TODO: Jump instructions take more cycles if the condition is true

const unsigned char instructions_ticks[256] = {
    4, 12, 8, 8, 4, 4, 8, 4,     20, 8, 8, 8, 4, 4, 8, 4, // 0x0_
    4, 12, 8, 8, 4, 4, 8, 4,     12, 8, 8, 8, 4, 4, 8, 4, // 0x1_
    8, 12, 8, 8, 4, 4, 8, 4,     8, 8, 8, 8, 4, 4, 8, 4, // 0x2_
    8, 12, 8, 8, 12, 12, 12, 4,  8, 8, 8, 8, 4, 4, 8, 4, // 0x3_

    4, 4, 4, 4, 4, 4, 8, 4,  4, 4, 4, 4, 4, 4, 8, 4, // 0x4_
    4, 4, 4, 4, 4, 4, 8, 4,  4, 4, 4, 4, 4, 4, 8, 4, // 0x5_
    4, 4, 4, 4, 4, 4, 8, 4,  4, 4, 4, 4, 4, 4, 8, 4, // 0x6_
    8, 8, 8, 8, 8, 8, 4, 8,  4, 4, 4, 4, 4, 4, 8, 4, // 0x7_

    4, 4, 4, 4, 4, 4, 8, 4,  4, 4, 4, 4, 4, 4, 8, 4, // 0x8_
    4, 4, 4, 4, 4, 4, 8, 4,  4, 4, 4, 4, 4, 4, 8, 4, // 0x9_
    4, 4, 4, 4, 4, 4, 8, 4,  4, 4, 4, 4, 4, 4, 8, 4, // 0xa_
    4, 4, 4, 4, 4, 4, 8, 4,  4, 4, 4, 4, 4, 4, 8, 4, // 0xb_

    8, 12, 12, 16, 12, 16, 8, 16,  8, 16, 12, 4, 12, 24, 8, 16, // 0xc_
    8, 12, 12,  0, 12, 16, 8, 16,  8, 16, 12, 0, 12,  0, 8, 16, // 0xd_
    12, 12, 8,  0, 0, 16, 8, 16,  16,  4, 16, 0,  0,  0, 8, 16, // 0xe_
    12, 12, 8,  4, 0, 16, 8, 16,  12,  8, 16, 4,  0,  0, 8, 16  // 0xf_
};

/*
 * Instructions with prefix CB
 */
const struct instruction instructions_cb[256] = {
    { "RLC B", rlc_op, &registers.b},           // 0x00
    { "RLC C", rlc_op, &registers.c},           // 0x01
    { "RLC D", rlc_op, &registers.d},           // 0x02
    { "RLC E", rlc_op, &registers.e},           // 0x03
    { "RLC H", rlc_op, &registers.h},           // 0x04
    { "RLC L", rlc_op, &registers.l},           // 0x05
    { "RLC (HL)", rlc_from_mem},      // 0x06
    { "RLC A", rlc_op, &registers.a},           // 0x07
    { "RRC B", rrc_op, &registers.b},           // 0x08
    { "RRC C", rrc_op, &registers.c},           // 0x09
    { "RRC D", rrc_op, &registers.d},           // 0x0a
    { "RRC E", rrc_op, &registers.e},           // 0x0b
    { "RRC H", rrc_op, &registers.h},           // 0x0c
    { "RRC L", rrc_op, &registers.l},           // 0x0d
    { "RRC (HL)", rrc_from_mem},      // 0x0e
    { "RRC A", rrc_op, &registers.a},           // 0x0f
    { "RL B", rl_op, &registers.b},             // 0x10
    { "RL C", rl_op, &registers.c},             // 0x11
    { "RL D", rl_op, &registers.d},             // 0x12
    { "RL E", rl_op, &registers.e},             // 0x13
    { "RL H", rl_op, &registers.h},             // 0x14
    { "RL L", rl_op, &registers.l},             // 0x15
    { "RL (HL)", rl_from_mem},        // 0x16
    { "RL A", rl_op, &registers.a},             // 0x17
    { "RR B", rr_op, &registers.b},             // 0x18
    { "RR C", rr_op, &registers.c},             // 0x19
    { "RR D", rr_op, &registers.d},             // 0x1a
    { "RR E", rr_op, &registers.e},             // 0x1b
    { "RR H", rr_op, &registers.h},             // 0x1c
    { "RR L", rr_op, &registers.l},             // 0x1d
    { "RR (HL)", rr_from_mem},        // 0x1e
    { "RR A", rr_op, &registers.a},             // 0x1f
    { "SLA B", sla_op, &registers.b},           // 0x20
    { "SLA C", sla_op, &registers.c},           // 0x21
    { "SLA D", sla_op, &registers.d},           // 0x22
    { "SLA E", sla_op, &registers.e},           // 0x23
    { "SLA H", sla_op, &registers.h},           // 0x24
    { "SLA L", sla_op, &registers.l},           // 0x25
    { "SLA (HL)", sla_from_mem},      // 0x26
    { "SLA A", sla_op, &registers.a},           // 0x27
    { "SRA B", sra_op, &registers.b},           // 0x28
    { "SRA C", sra_op, &registers.c},           // 0x29
    { "SRA D", sra_op, &registers.d},           // 0x2a
    { "SRA E", sra_op, &registers.e},           // 0x2b
    { "SRA H", sra_op, &registers.h},           // 0x2c
    { "SRA L", sra_op, &registers.l},           // 0x2d
    { "SRA (HL)", sra_from_mem},      // 0x2e
    { "SRA A", sra_op, &registers.a},           // 0x2f
    { "SWAP B", swap, &registers.b},         // 0x30
    { "SWAP C", swap, &registers.c},         // 0x31
    { "SWAP D", swap, &registers.d},         // 0x32
    { "SWAP E", swap, &registers.e},         // 0x33
    { "SWAP H", swap, &registers.h},         // 0x34
    { "SWAP L", swap, &registers.l},         // 0x35
    { "SWAP (HL)", swap_from_mem},    // 0x36
    { "SWAP A", swap, &registers.a},         // 0x37
    { "SRL B", srl_op, &registers.b},           // 0x38
    { "SRL C", srl_op, &registers.c},           // 0x39
    { "SRL D", srl_op, &registers.d},           // 0x3a
    { "SRL E", srl_op, &registers.e},           // 0x3b
    { "SRL H", srl_op, &registers.h},           // 0x3c
    { "SRL L", srl_op, &registers.l},           // 0x3d
    { "SRL (HL)", srl_from_mem},      // 0x3e
    { "SRL A", srl_op, &registers.a},           // 0x3f
    { "BIT 0, B", bit_op, (void*) 0, &registers.b },      // 0x40
    { "BIT 0, C", bit_op, (void*) 0, &registers.c },      // 0x41
    { "BIT 0, D", bit_op, (void*) 0, &registers.d },      // 0x42
    { "BIT 0, E", bit_op, (void*) 0, &registers.e },      // 0x43
    { "BIT 0, H", bit_op, (void*) 0, &registers.h },      // 0x44
    { "BIT 0, L", bit_op, (void*) 0, &registers.l },      // 0x45
    { "BIT 0, (HL)", bit_op_from_mem, (void*) 0, &registers.hl }, // 0x46
    { "BIT 0, A", bit_op, (void*) 0, &registers.a },      // 0x47
    { "BIT 1, B", bit_op, (void*) 1, &registers.b },      // 0x48
    { "BIT 1, C", bit_op, (void*) 1, &registers.c },      // 0x49
    { "BIT 1, D", bit_op, (void*) 1, &registers.d },      // 0x4a
    { "BIT 1, E", bit_op, (void*) 1, &registers.e },      // 0x4b
    { "BIT 1, H", bit_op, (void*) 1, &registers.h },      // 0x4c
    { "BIT 1, L", bit_op, (void*) 1, &registers.l },      // 0x4d
    { "BIT 1, (HL)", bit_op_from_mem, (void*) 1, &registers.hl }, // 0x4e
    { "BIT 1, A", bit_op, (void*) 1, &registers.a },      // 0x4f
    { "BIT 2, B", bit_op, (void*) 2, &registers.b },      // 0x50
    { "BIT 2, C", bit_op, (void*) 2, &registers.c },      // 0x51
    { "BIT 2, D", bit_op, (void*) 2, &registers.d },      // 0x52
    { "BIT 2, E", bit_op, (void*) 2, &registers.e },      // 0x53
    { "BIT 2, H", bit_op, (void*) 2, &registers.h },      // 0x54
    { "BIT 2, L", bit_op, (void*) 2, &registers.l },      // 0x55
    { "BIT 2, (HL)", bit_op_from_mem, (void*) 2, &registers.hl }, // 0x56
    { "BIT 2, A", bit_op, (void*) 2, &registers.a },      // 0x57
    { "BIT 3, B", bit_op, (void*) 3, &registers.b },      // 0x58
    { "BIT 3, C", bit_op, (void*) 3, &registers.c },      // 0x59
    { "BIT 3, D", bit_op, (void*) 3, &registers.d },      // 0x5a
    { "BIT 3, E", bit_op, (void*) 3, &registers.e },      // 0x5b
    { "BIT 3, H", bit_op, (void*) 3, &registers.h },      // 0x5c
    { "BIT 3, L", bit_op, (void*) 3, &registers.l },      // 0x5d
    { "BIT 3, (HL)", bit_op_from_mem, (void*) 3, &registers.hl }, // 0x5e
    { "BIT 3, A", bit_op, (void*) 3, &registers.a },      // 0x5f
    { "BIT 4, B", bit_op, (void*) 4, &registers.b },      // 0x60
    { "BIT 4, C", bit_op, (void*) 4, &registers.c },      // 0x61
    { "BIT 4, D", bit_op, (void*) 4, &registers.d },      // 0x62
    { "BIT 4, E", bit_op, (void*) 4, &registers.e },      // 0x63
    { "BIT 4, H", bit_op, (void*) 4, &registers.h },      // 0x64
    { "BIT 4, L", bit_op, (void*) 4, &registers.l },      // 0x65
    { "BIT 4, (HL)", bit_op_from_mem, (void*) 4, &registers.hl }, // 0x66
    { "BIT 4, A", bit_op, (void*) 4, &registers.a },      // 0x67
    { "BIT 5, B", bit_op, (void*) 5, &registers.b },      // 0x68
    { "BIT 5, C", bit_op, (void*) 5, &registers.c },      // 0x69
    { "BIT 5, D", bit_op, (void*) 5, &registers.d },      // 0x6a
    { "BIT 5, E", bit_op, (void*) 5, &registers.e },      // 0x6b
    { "BIT 5, H", bit_op, (void*) 5, &registers.h },      // 0x6c
    { "BIT 5, L", bit_op, (void*) 5, &registers.l },      // 0x6d
    { "BIT 5, (HL)", bit_op_from_mem, (void*) 5, &registers.hl }, // 0x6e
    { "BIT 5, A", bit_op, (void*) 5, &registers.a },      // 0x6f
    { "BIT 6, B", bit_op, (void*) 6, &registers.b },      // 0x70
    { "BIT 6, C", bit_op, (void*) 6, &registers.c },      // 0x71
    { "BIT 6, D", bit_op, (void*) 6, &registers.d },      // 0x72
    { "BIT 6, E", bit_op, (void*) 6, &registers.e },      // 0x73
    { "BIT 6, H", bit_op, (void*) 6, &registers.h },      // 0x74
    { "BIT 6, L", bit_op, (void*) 6, &registers.l },      // 0x75
    { "BIT 6, (HL)", bit_op_from_mem, (void*) 6, &registers.hl}, // 0x76
    { "BIT 6, A", bit_op, (void*) 6, &registers.a },      // 0x77
    { "BIT 7, B", bit_op, (void*) 7, &registers.b },      // 0x78
    { "BIT 7, C", bit_op, (void*) 7, &registers.c },      // 0x79
    { "BIT 7, D", bit_op, (void*) 7, &registers.d },      // 0x7a
    { "BIT 7, E", bit_op, (void*) 7, &registers.e },      // 0x7b
    { "BIT 7, H", bit_op, (void*) 7, &registers.h },      // 0x7c
    { "BIT 7, L", bit_op, (void*) 7, &registers.l },      // 0x7d
    { "BIT 7, (HL)", bit_op_from_mem, (void*) 7, &registers.hl }, // 0x7e
    { "BIT 7, A", bit_op, (void*) 7, &registers.a },      // 0x7f
    { "RES 0, B", res_op, (void*) 0, &registers.b},      // 0x80
    { "RES 0, C", res_op, (void*) 0, &registers.c},      // 0x81
    { "RES 0, D", res_op, (void*) 0, &registers.d},      // 0x82
    { "RES 0, E", res_op, (void*) 0, &registers.e},      // 0x83
    { "RES 0, H", res_op, (void*) 0, &registers.h},      // 0x84
    { "RES 0, L", res_op, (void*) 0, &registers.l},      // 0x85
    { "RES 0, (HL)", res_from_mem, (void*) 0, &registers.hl}, // 0x86
    { "RES 0, A", res_op, (void*) 0, &registers.a},      // 0x87
    { "RES 1, B", res_op, (void*) 1, &registers.b},      // 0x88
    { "RES 1, C", res_op, (void*) 1, &registers.c},      // 0x89
    { "RES 1, D", res_op, (void*) 1, &registers.d},      // 0x8a
    { "RES 1, E", res_op, (void*) 1, &registers.e},      // 0x8b
    { "RES 1, H", res_op, (void*) 1, &registers.h},      // 0x8c
    { "RES 1, L", res_op, (void*) 1, &registers.l},      // 0x8d
    { "RES 1, (HL)", res_from_mem, (void*) 1, &registers.hl}, // 0x8e
    { "RES 1, A", res_op, (void*) 1, &registers.a},      // 0x8f
    { "RES 2, B", res_op, (void*) 2, &registers.b},      // 0x90
    { "RES 2, C", res_op, (void*) 2, &registers.c},      // 0x91
    { "RES 2, D", res_op, (void*) 2, &registers.d},      // 0x92
    { "RES 2, E", res_op, (void*) 2, &registers.e},      // 0x93
    { "RES 2, H", res_op, (void*) 2, &registers.h},      // 0x94
    { "RES 2, L", res_op, (void*) 2, &registers.l},      // 0x95
    { "RES 2, (HL)", res_from_mem, (void*) 2, &registers.hl}, // 0x96
    { "RES 2, A", res_op, (void*) 2, &registers.a},      // 0x97
    { "RES 3, B", res_op, (void*) 3, &registers.b},      // 0x98
    { "RES 3, C", res_op, (void*) 3, &registers.c},      // 0x99
    { "RES 3, D", res_op, (void*) 3, &registers.d},      // 0x9a
    { "RES 3, E", res_op, (void*) 3, &registers.e},      // 0x9b
    { "RES 3, H", res_op, (void*) 3, &registers.h},      // 0x9c
    { "RES 3, L", res_op, (void*) 3, &registers.l},      // 0x9d
    { "RES 3, (HL)", res_from_mem, (void*) 3, &registers.hl}, // 0x9e
    { "RES 3, A", res_op, (void*) 3, &registers.a},      // 0x9f
    { "RES 4, B", res_op, (void*) 4, &registers.b},      // 0xa0
    { "RES 4, C", res_op, (void*) 4, &registers.c},      // 0xa1
    { "RES 4, D", res_op, (void*) 4, &registers.d},      // 0xa2
    { "RES 4, E", res_op, (void*) 4, &registers.e},      // 0xa3
    { "RES 4, H", res_op, (void*) 4, &registers.h},      // 0xa4
    { "RES 4, L", res_op, (void*) 4, &registers.l},      // 0xa5
    { "RES 4, (HL)", res_from_mem, (void*) 4, &registers.hl}, // 0xa6
    { "RES 4, A", res_op, (void*) 4, &registers.a},      // 0xa7
    { "RES 5, B", res_op, (void*) 5, &registers.b},      // 0xa8
    { "RES 5, C", res_op, (void*) 5, &registers.c},      // 0xa9
    { "RES 5, D", res_op, (void*) 5, &registers.d},      // 0xaa
    { "RES 5, E", res_op, (void*) 5, &registers.e},      // 0xab
    { "RES 5, H", res_op, (void*) 5, &registers.h},      // 0xac
    { "RES 5, L", res_op, (void*) 5, &registers.l},      // 0xad
    { "RES 5, (HL)", res_from_mem, (void*) 5, &registers.hl}, // 0xae
    { "RES 5, A", res_op, (void*) 5, &registers.a},      // 0xaf
    { "RES 6, B", res_op, (void*) 6, &registers.b},      // 0xb0
    { "RES 6, C", res_op, (void*) 6, &registers.c},      // 0xb1
    { "RES 6, D", res_op, (void*) 6, &registers.d},      // 0xb2
    { "RES 6, E", res_op, (void*) 6, &registers.e},      // 0xb3
    { "RES 6, H", res_op, (void*) 6, &registers.h},      // 0xb4
    { "RES 6, L", res_op, (void*) 6, &registers.l},      // 0xb5
    { "RES 6, (HL)", res_from_mem, (void*) 6, &registers.hl}, // 0xb6
    { "RES 6, A", res_op, (void*) 6, &registers.a},      // 0xb7
    { "RES 7, B", res_op, (void*) 7, &registers.b},      // 0xb8
    { "RES 7, C", res_op, (void*) 7, &registers.c},      // 0xb9
    { "RES 7, D", res_op, (void*) 7, &registers.d},      // 0xba
    { "RES 7, E", res_op, (void*) 7, &registers.e},      // 0xbb
    { "RES 7, H", res_op, (void*) 7, &registers.h},      // 0xbc
    { "RES 7, L", res_op, (void*) 7, &registers.l},      // 0xbd
    { "RES 7, (HL)", res_from_mem, (void*) 7, &registers.hl}, // 0xbe
    { "RES 7, A", res_op, (void*) 7, &registers.a},      // 0xbf
    { "SET 0, B", set_op, (void*) 0, &registers.b },      // 0xc0
    { "SET 0, C", set_op, (void*) 0, &registers.c },      // 0xc1
    { "SET 0, D", set_op, (void*) 0, &registers.d },      // 0xc2
    { "SET 0, E", set_op, (void*) 0, &registers.e },      // 0xc3
    { "SET 0, H", set_op, (void*) 0, &registers.h },      // 0xc4
    { "SET 0, L", set_op, (void*) 0, &registers.l },      // 0xc5
    { "SET 0, (HL)", set_op_from_mem, (void*) 0, &registers.hl }, // 0xc6
    { "SET 0, A", set_op, (void*) 0, &registers.a },      // 0xc7
    { "SET 1, B", set_op, (void*) 1, &registers.b },      // 0xc8
    { "SET 1, C", set_op, (void*) 1, &registers.c },      // 0xc9
    { "SET 1, D", set_op, (void*) 1, &registers.d },      // 0xca
    { "SET 1, E", set_op, (void*) 1, &registers.e },      // 0xcb
    { "SET 1, H", set_op, (void*) 1, &registers.h },      // 0xcc
    { "SET 1, L", set_op, (void*) 1, &registers.l },      // 0xcd
    { "SET 1, (HL)", set_op_from_mem, (void*) 1, &registers.hl }, // 0xce
    { "SET 1, A", set_op, (void*) 1, &registers.a },      // 0xcf
    { "SET 2, B", set_op, (void*) 2, &registers.b },      // 0xd0
    { "SET 2, C", set_op, (void*) 2, &registers.c },      // 0xd1
    { "SET 2, D", set_op, (void*) 2, &registers.d },      // 0xd2
    { "SET 2, E", set_op, (void*) 2, &registers.e },      // 0xd3
    { "SET 2, H", set_op, (void*) 2, &registers.h },      // 0xd4
    { "SET 2, L", set_op, (void*) 2, &registers.l },      // 0xd5
    { "SET 2, (HL)", set_op_from_mem, (void*) 2, &registers.hl }, // 0xd6
    { "SET 2, A", set_op, (void*) 2, &registers.a },      // 0xd7
    { "SET 3, B", set_op, (void*) 3, &registers.b },      // 0xd8
    { "SET 3, C", set_op, (void*) 3, &registers.c },      // 0xd9
    { "SET 3, D", set_op, (void*) 3, &registers.d },      // 0xda
    { "SET 3, E", set_op, (void*) 3, &registers.e },      // 0xdb
    { "SET 3, H", set_op, (void*) 3, &registers.h },      // 0xdc
    { "SET 3, L", set_op, (void*) 3, &registers.l },      // 0xdd
    { "SET 3, (HL)", set_op_from_mem, (void*) 3, &registers.hl }, // 0xde
    { "SET 3, A", set_op, (void*) 3, &registers.a },      // 0xdf
    { "SET 4, B", set_op, (void*) 4, &registers.b },      // 0xe0
    { "SET 4, C", set_op, (void*) 4, &registers.c },      // 0xe1
    { "SET 4, D", set_op, (void*) 4, &registers.d },      // 0xe2
    { "SET 4, E", set_op, (void*) 4, &registers.e },      // 0xe3
    { "SET 4, H", set_op, (void*) 4, &registers.h },      // 0xe4
    { "SET 4, L", set_op, (void*) 4, &registers.l },      // 0xe5
    { "SET 4, (HL)", set_op_from_mem, (void*) 4, &registers.hl }, // 0xe6
    { "SET 4, A", set_op, (void*) 4, &registers.a },      // 0xe7
    { "SET 5, B", set_op, (void*) 5, &registers.b },      // 0xe8
    { "SET 5, C", set_op, (void*) 5, &registers.c },      // 0xe9
    { "SET 5, D", set_op, (void*) 5, &registers.d },      // 0xea
    { "SET 5, E", set_op, (void*) 5, &registers.e },      // 0xeb
    { "SET 5, H", set_op, (void*) 5, &registers.h },      // 0xec
    { "SET 5, L", set_op, (void*) 5, &registers.l },      // 0xed
    { "SET 5, (HL)", set_op_from_mem, (void*) 5, &registers.hl }, // 0xee
    { "SET 5, A", set_op, (void*) 5, &registers.a },      // 0xef
    { "SET 6, B", set_op, (void*) 6, &registers.b },      // 0xf0
    { "SET 6, C", set_op, (void*) 6, &registers.c },      // 0xf1
    { "SET 6, D", set_op, (void*) 6, &registers.d },      // 0xf2
    { "SET 6, E", set_op, (void*) 6, &registers.e },      // 0xf3
    { "SET 6, H", set_op, (void*) 6, &registers.h },      // 0xf4
    { "SET 6, L", set_op, (void*) 6, &registers.l },      // 0xf5
    { "SET 6, (HL)", set_op_from_mem, (void*) 6, &registers.hl }, // 0xf6
    { "SET 6, A", set_op, (void*) 6, &registers.a },      // 0xf7
    { "SET 7, B", set_op, (void*) 7, &registers.b },      // 0xf8
    { "SET 7, C", set_op, (void*) 7, &registers.c },      // 0xf9
    { "SET 7, D", set_op, (void*) 7, &registers.d },      // 0xfa
    { "SET 7, E", set_op, (void*) 7, &registers.e },      // 0xfb
    { "SET 7, H", set_op, (void*) 7, &registers.h },      // 0xfc
    { "SET 7, L", set_op, (void*) 7, &registers.l },      // 0xfd
    { "SET 7, (HL)", set_op_from_mem, (void*) 7, &registers.hl}, // 0xfe
    { "SET 7, A", set_op, (void*) 7, &registers.a},      // 0xff
};

const unsigned char instructions_cb_ticks[256] = {
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x0_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x1_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x2_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x3_
    8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x4_
    8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x5_
    8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x6_
    8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x7_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x8_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x9_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0xa_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0xb_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0xc_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0xd_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0xe_
    8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8  // 0xf_
};





/*---- Interrupts -------------------------------------------------*/


#define VBLANK_INTERRUPT ((unsigned char) 1) // 0000 0001
#define LCDSTAT_INTERRUPT ((unsigned char) 2) // 0000 0010
#define TIMER_INTERRUPT ((unsigned char) 4) // 0000 0100
#define SERIAL_INTERRUPT ((unsigned char) 8) // 0000 1000
#define JOYPAD_INTERRUPT ((unsigned char) 16) // 0001 0000

void request_interrupt(unsigned char interrupt_flag) {

    *interrupt_request_register |= interrupt_flag;
}

static void process_interrupts() {

    unsigned char test_mask = 1;

    for (int i=0; i<5; i++) {

        /*   if there's an interrupt request, and that interrupt is "enabled" in the
         * interrupt enable register (which is set by the game), then the request is acknowledged
         * and processed.
         */
        if ( (*interrupt_request_register & test_mask) & *interrupt_enable_register ) {

            // When an interrupt is requested, the cpu is no longer halted;
            halted = 0;

            if ( interrupt_master_enable ) {

                disable_interrupts();

                /*   Acknowledge the request:
                     clear the interrupt request bit that triggered the request */
                *interrupt_request_register &= ~test_mask;

                /*   Call the interruption handler
                 * (Interruption handlers are in addresses 0x40 to 0x60) */
                unsigned short address = 0x40 + 0x8*i;
                call(address);
            
            }

            /* The interruption handler will return and re-enable interrupts */

            break;
        }

        test_mask <<= 1;

    }


    check_disable_bootrom();

}





/*---- Main Logic and Execution -----------------------------------*/

static int execute() {

    /* printf("A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X (%02X %02X %02X %02X)\n", registers.a, registers.f, registers.b, registers.c, registers.d, registers.e, registers.h, registers.l, registers.sp, registers.pc, memory[registers.pc], memory[registers.pc+1], memory[registers.pc+2], memory[registers.pc+3]); */

    /* (*lcd_ly) = 0x90; // Stubbed for testing purposes; */

    unsigned char opcode = memory[registers.pc++];

    if (opcode == 0xe4)
        printf("UNKNOWN CODE IN PC: %x\n", registers.pc-1);

    int time;   /* time is in cycles */
    extra_instruction_cycles = 0;
    struct instruction instruction;

    if (opcode == 0xcb) {       /* if op is CB prefix, execute next op from the CB instruction set */
        opcode = memory[registers.pc++];
        instruction = instructions_cb[opcode];
        time = instructions_cb_ticks[opcode];
    }
    else {
        instruction = instructions[opcode];
        time = instructions_ticks[opcode];
    }

    if (instruction.execute) {

        if (debugger)
            printf("%s -> 0x%x\n", instruction.disassembly, opcode);

        instruction.execute(instruction.exec_argv1, instruction.exec_argv2);

        if (debugger)
            debug();

    } else {

        printf("Operation not defined: %s -> 0x%x\n", instruction.disassembly, opcode);
        exit(1);

    }


    return time + extra_instruction_cycles;
}


int cpu() {

    if (stopped)    /* when the CPU is stopped, just keep updating the window (return random number of cycles) */
        return 1;
    int cycles = 0;

    if (!halted)
        cycles = execute();
    else
        cycles = 4;

    process_interrupts();

    return cycles;
}

void boot_tests() {
    registers.af = 0x01B0;
    registers.bc = 0x0013;
    registers.de = 0x00D8;
    registers.hl = 0x014D;
    registers.sp = 0xfffe;
    registers.pc = 0x100;
}
