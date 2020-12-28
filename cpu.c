/*
 *  Gameboy Emulator: CPU
 *
 *
 *  Resources:
 *  
 *  > Everything ~you wanted to know
 *  http://bgb.bircd.org/pandocs.htm
 *
 *  > Instruction Set + Registers
 *  https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
 *
 *  > More Opcodes
 *  http://gameboy.mongenel.com/dmg/opcodes.html
 *
 *  
 *  Notes:
 *
 *  Load memory.c before loading this file (cpu.c)
 *
 */




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
    unsigned short pc;
};

// Registers are global
struct registers registers = {{{0}}};
int interrupts = 0;


/*
 *  Flags operations
 */

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



/*
 *  CPU Operations
 */

/*
 * =============
 *  CPU Utils 
 * =============
 */

static unsigned char read8bit_operand() {
    printf("8-bit read: %d\n", memory[registers.pc]);
    return memory[registers.pc++];
}

static unsigned short read16bit_operand() {

    unsigned char operand_low = memory[registers.pc++];
    unsigned char operand_high = memory[registers.pc++];

    unsigned short operand = 0 | operand_high;
    operand = (operand << 8) | operand_low;

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


/*
 * =============
 *  8-Bit Loads
 * =============
 */

static void load8bit(unsigned char * destination, unsigned char * source) { 
    *destination = *source;
}

static void load8bit_operand(unsigned char * destination, void* _unused) { 

    unsigned char operand = read8bit_operand();

    load8bit(destination, &operand);
}

static void load8bit_to_mem(unsigned short * reg_with_pointer, unsigned char * source) { 
    load8bit(&memory[*reg_with_pointer], source);
}

static void load8bit_from_mem(unsigned char* destination, unsigned short* reg_with_pointer) {
    load8bit(destination, &memory[*reg_with_pointer]);
}

static void load8bit_dec_to_mem(unsigned short * reg_with_pointer) { 

    load8bit(&memory[*reg_with_pointer], &registers.a);

    (*reg_with_pointer)--;
}

static void load8bit_to_mem_with_offset(unsigned char* offset, unsigned char* source) {
    
    unsigned short address = 0xFF00 + *offset;

    load8bit_to_mem(&address, source);
}

static void load8bit_to_mem_with_offset_operand(unsigned char* source, void* _unused) {
    
    unsigned char operand = read8bit_operand();

    load8bit_to_mem_with_offset(&operand, source);
}

static void load8bit_inc_to_mem(unsigned short* reg_with_pointer) {
	
	load8bit(&memory[*reg_with_pointer], &registers.a);

	(*reg_with_pointer)++;
}

/*
 * =============
 *  16-Bit Loads
 * ==============
 */

static void load16bit(unsigned short * destination, unsigned short *source) { 
    *destination = *source;
}

static void load16bit_operand(unsigned short * destination, void* _unused) {

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

}

/*
 * ===========
 *  8-Bit ALU
 * ===========
 */

static void add8bit(unsigned char* s) {

    registers.a += *s; 

    // zero flag 
    if (registers.a == 0) set_flag(FLAG_Z);
    else clear_flag(FLAG_Z);

    // add/sub flag
    clear_flag(FLAG_N);

    // half carry flag
    // (h is set if there's an overflow from the lowest 4 bits to the highest 4)
    if ((*s & 0xF) + (registers.a & 0xF) > 0xF) set_flag(FLAG_H);
    else clear_flag(FLAG_H);

    // carry flag
    if (registers.a < *s) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);

}

static void add8bit_from_mem(unsigned char* reg_with_pointer, void* _unused) { add8bit(&memory[*reg_with_pointer]); }

static void adc (unsigned char* regs) {
	unsigned char value = *regs;
	if(registers.f & (1<<3)) value++;
	add8bit(&value);
}

static void adc_from_mem(unsigned char* reg_with_pointer) {
    adc(&memory[*reg_with_pointer]);
}

static void xor_reg(unsigned char* reg) { 

    registers.a ^= *reg;

    if (registers.a == 0)
        set_flag(FLAG_Z);
    else
        clear_flag(FLAG_Z);

    clear_flag(FLAG_N);
    clear_flag(FLAG_H);
    clear_flag(FLAG_CY);

}

static void xor_reg_from_mem(unsigned short * reg_with_pointer) {
    xor_reg(&memory[*reg_with_pointer]);
}

static void inc8bit(unsigned char* reg) {

    (*reg)++;

    if (*reg==0) set_flag(FLAG_Z);
    else clear_flag(FLAG_Z);

    clear_flag(FLAG_N);

    if ( (1 & 0xF) + (*reg & 0xF) > 0xF ) set_flag(FLAG_H);
    else clear_flag(FLAG_H);

}

static void dec8bit(unsigned char* reg) {

	(*reg)--;

	if (*reg==0) set_flag(FLAG_Z);
	else clear_flag(FLAG_Z);

	set_flag(FLAG_N);

	if ( (1 & 0xF) + (*reg & 0xF) > 0xF ) set_flag(FLAG_H);
	else clear_flag(FLAG_H);
}	

static void cp_operand() {

	unsigned char s = read8bit_operand();
	unsigned char cp_a = registers.a - s;

    // zero flag 
    if (cp_a == 0) set_flag(FLAG_Z);
    else clear_flag(FLAG_Z);

    // add/sub flag
    set_flag(FLAG_N);

    // half carry flag
    // (h is set if there's an overflow from the lowest 4 bits to the highest 4)
    if ((s & 0xF) + (cp_a & 0xF) > 0xF) set_flag(FLAG_H);
    else clear_flag(FLAG_H);

    // carry flag
    if (cp_a < s) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);
}

/*
 * =============
 *  16-Bit Loads
 * ==============
 */

static void add16bit(unsigned short* source) {
	
	unsigned short to_add = *source;
	unsigned short add_bit = registers.hl + to_add;
	registers.hl = add_bit;

	clear_flag(FLAG_N);

	//TODO : verify half-carry flag (from low 8 to high 8)

	if ( (to_add & 0xFF) + (add_bit & 0xFF) > 0xFF ) set_flag(FLAG_H);
	else clear_flag(FLAG_H);

	if (add_bit < to_add) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);
}



/*
 * ==================
 *  Rotates & Shifts
 * ==================
 */

static void rl_op(unsigned char* reg) {

    // check if carry is set
    unsigned char hadcarry = 0;
	if (registers.f & (1<<3)) hadcarry++;

    // check highest bit from reg to check if carry should be set
    if ((*reg) & 0x80 > 0) set_flag(FLAG_CY);
    else clear_flag(FLAG_CY);

    // rotate
    *reg <<= *reg;

    // if had carry, set it to the lowest bit
    if (hadcarry) *reg |= 0x01;
    
    clear_flag(FLAG_N);
    clear_flag(FLAG_H);

}

static void rla_op() {

    rl_op(&registers.a);
    clear_flag(FLAG_Z);
}


/*
 * =============
 *  Bit Opcodes 
 * =============
 */

static void bit_op(void* n, unsigned char * reg) {

    unsigned char n_byte = (unsigned char) n;

    if ( *reg & (1 << n_byte)  ) set_flag(FLAG_Z);
    else clear_flag(FLAG_Z);

    clear_flag(FLAG_N);
    set_flag(FLAG_H);
}

static void set_op(void* n, unsigned char* reg) {

	unsigned char n_byte = (unsigned char) n;
	
	*reg |= (1 << (n_byte)); 
}

static void set_op_from_mem(void* n , unsigned char* reg_with_pointer) {
    set_op(n,&memory[*reg_with_pointer]);
}

/*
 * =============
 *  Jumps
 * =============
 */

static void jump_operand(void* unused , void* unused2) {
	registers.pc = read16bit_operand();
}

// Sets program counter to operand
static void jump_condition(void* flag, void* jump_cond) {

    unsigned char flagb = (unsigned char) flag;
    unsigned char jump_condb = (unsigned char) jump_cond;

    unsigned char flag_status = (flagb & registers.f);
    if ( (flag_status > 0 && jump_condb != 0 ) || ((flag_status) == 0 && jump_condb == 0) )
        registers.pc = read16bit_operand();

}

// Adds operand to the current program counter
static void jump_condition_add(void* flag, void* jump_cond) {

    unsigned char flagb = (unsigned char) flag;
    unsigned char jump_condb = (unsigned char) jump_cond;

    unsigned char flag_status = (flagb & registers.f);
    if ( (flag_status > 0 && jump_condb != 0 ) || ((flag_status) == 0 && jump_condb == 0) )
        registers.pc += read16bit_operand();

}

/*
 * =============
 *  Calls 
 * =============
 */

static void call_operand() {

    unsigned short operand = read16bit_operand();

    unsigned char pc_high = (registers.pc >> 8) & 0xFF;
    unsigned char pc_low = registers.pc & 0xFF;

    memory[--registers.sp] = pc_high;
    memory[--registers.sp] = pc_low;

    registers.pc = operand; 
    
}

/*
 * ===============
 *  Miscellaneous 
 * ===============
 */

static void nop () {
    // do nothing
}

static void enable_interrupts() {
    interrupts = 1;
}

// Handle instructions with prefix CB
void execute_cb();

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
	{ "LD BC, 0x%04X", NULL},                // 0x01
	{ "LD (BC), A", NULL},                   // 0x02
	{ "INC BC", NULL},                       // 0x03
	{ "INC B", NULL},                        // 0x04
	{ "DEC B", dec8bit, &registers.b},                        // 0x05
	{ "LD B, 0x%02X", load8bit_operand, &registers.b },                 // 0x06
	{ "RLCA", NULL},                         // 0x07
	{ "LD (0x%04X), SP", NULL},              // 0x08
	{ "ADD HL, BC", NULL},                   // 0x09
	{ "LD A, (BC)", NULL},                   // 0x0a
	{ "DEC BC", NULL},                       // 0x0b
	{ "INC C", inc8bit, &registers.c },                        // 0x0c
	{ "DEC C", NULL},                        // 0x0d
	{ "LD C, 0x%02X", load8bit_operand, &registers.c },                 // 0x0e
	{ "RRCA", NULL},                         // 0x0f
	{ "STOP", NULL},                         // 0x10
	{ "LD DE, 0x%04X", load16bit_operand, &registers.de },                // 0x11
	{ "LD (DE), A", NULL},                   // 0x12
	{ "INC DE", NULL},                       // 0x13
	{ "INC D", NULL},                        // 0x14
	{ "DEC D", NULL},                        // 0x15
	{ "LD D, 0x%02X", NULL},                 // 0x16
	{ "RLA", rl_op, &registers.a},                          // 0x17
	{ "JR 0x%02X", NULL},                    // 0x18
	{ "ADD HL, DE", NULL},                   // 0x19
	{ "LD A, (DE)", load8bit_from_mem, &registers.a, &registers.de },                   // 0x1a
	{ "DEC DE", NULL},                       // 0x1b
	{ "INC E", NULL},                        // 0x1c
	{ "DEC E", dec8bit, &registers.e},                        // 0x1d
	{ "LD E, 0x%02X", NULL},                 // 0x1e
	{ "RRA", NULL},                          // 0x1f
	{ "JR NZ, 0x%02X", jump_condition_add, (void*) FLAG_Z, 0 },                // 0x20
	{ "LD HL, 0x%04X", load16bit_operand, &registers.hl },                // 0x21
	{ "LDI (HL), A", load8bit_inc_to_mem, &registers.hl},                  // 0x22
	{ "INC HL", NULL},                       // 0x23
	{ "INC H", NULL},                        // 0x24
	{ "DEC H", dec8bit, &registers.h},                        // 0x25
	{ "LD H, 0x%02X", NULL},                 // 0x26
	{ "DAA", NULL},                          // 0x27
	{ "JR Z, 0x%02X", NULL},                 // 0x28
	{ "ADD HL, HL", add16bit, &registers.hl},                   // 0x29
	{ "LDI A, (HL)", NULL},                  // 0x2a
	{ "DEC HL", NULL},                       // 0x2b
	{ "INC L", NULL},                        // 0x2c
	{ "DEC L", NULL},                        // 0x2d
	{ "LD L, 0x%02X", NULL},                 // 0x2e
	{ "CPL", NULL},                          // 0x2f
	{ "JR NC, 0x%02X", NULL},                // 0x30
	{ "LD SP, 0x%04X", load16bit_operand, &registers.sp},             // 0x31
	{ "LDD (HL), A", load8bit_dec_to_mem, &registers.hl},                  // 0x32
	{ "INC SP", NULL},                       // 0x33
	{ "INC (HL)", NULL},                     // 0x34
	{ "DEC (HL)", NULL},                     // 0x35
	{ "LD (HL), 0x%02X", NULL},              // 0x36
	{ "SCF", NULL},                          // 0x37
	{ "JR C, 0x%02X", NULL},                 // 0x38
	{ "ADD HL, SP", NULL},                   // 0x39
	{ "LDD A, (HL)", NULL},                  // 0x3a
	{ "DEC SP", NULL},                       // 0x3b
	{ "INC A", NULL},                        // 0x3c
	{ "DEC A", NULL},                        // 0x3d
	{ "LD A, 0x%02X", load8bit_operand, &registers.hl, &registers.a},                 // 0x3e
	{ "CCF", NULL},                          // 0x3f
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
	{ "HALT", NULL},                         // 0x76
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
	{ "ADD A, (HL)", add8bit_from_mem, &registers.hl},                  // 0x86
	{ "ADD A", add8bit, &registers.a},                        // 0x87
	{ "ADC B", adc, &registers.b},                        // 0x88
	{ "ADC C", adc, &registers.c},                        // 0x89
	{ "ADC D", adc, &registers.d},                        // 0x8a
	{ "ADC E", adc, &registers.e},                        // 0x8b
	{ "ADC H", adc, &registers.h},                        // 0x8c
	{ "ADC L", adc, &registers.l},                        // 0x8d
	{ "ADC (HL)", adc_from_mem, &registers.hl},                     // 0x8e
	{ "ADC A", adc, &registers.a},                        // 0x8f
	{ "SUB B", NULL},                        // 0x90
	{ "SUB C", NULL},                        // 0x91
	{ "SUB D", NULL},                        // 0x92
	{ "SUB E", NULL},                        // 0x93
	{ "SUB H", NULL},                        // 0x94
	{ "SUB L", NULL},                        // 0x95
	{ "SUB (HL)", NULL},                     // 0x96
	{ "SUB A", NULL},                        // 0x97
	{ "SBC B", NULL},                        // 0x98
	{ "SBC C", NULL},                        // 0x99
	{ "SBC D", NULL},                        // 0x9a
	{ "SBC E", NULL},                        // 0x9b
	{ "SBC H", NULL},                        // 0x9c
	{ "SBC L", NULL},                        // 0x9d
	{ "SBC (HL)", NULL},                     // 0x9e
	{ "SBC A", NULL},                        // 0x9f
	{ "AND B", NULL},                        // 0xa0
	{ "AND C", NULL},                        // 0xa1
	{ "AND D", NULL},                        // 0xa2
	{ "AND E", NULL},                        // 0xa3
	{ "AND H", NULL},                        // 0xa4
	{ "AND L", NULL},                        // 0xa5
	{ "AND (HL)", NULL},                     // 0xa6
	{ "AND A", NULL},                        // 0xa7
	{ "XOR B", xor_reg, &registers.b},                        // 0xa8
	{ "XOR C", xor_reg, &registers.c},                        // 0xa9
	{ "XOR D", xor_reg, &registers.d},                        // 0xaa
	{ "XOR E", xor_reg, &registers.e},                        // 0xab
	{ "XOR H", xor_reg, &registers.h},                        // 0xac
	{ "XOR L", xor_reg, &registers.l},                        // 0xad
	{ "XOR (HL)", xor_reg_from_mem, &registers.hl},                     // 0xae
	{ "XOR A", xor_reg, &registers.a},                        // 0xaf
	{ "OR B", NULL},                         // 0xb0
	{ "OR C", NULL},                         // 0xb1
	{ "OR D", NULL},                         // 0xb2
	{ "OR E", NULL},                         // 0xb3
	{ "OR H", NULL},                         // 0xb4
	{ "OR L", NULL},                         // 0xb5
	{ "OR (HL)", NULL},                      // 0xb6
	{ "OR A", NULL},                         // 0xb7
	{ "CP B", NULL},                         // 0xb8
	{ "CP C", NULL},                         // 0xb9
	{ "CP D", NULL},                         // 0xba
	{ "CP E", NULL},                         // 0xbb
	{ "CP H", NULL},                         // 0xbc
	{ "CP L", NULL},                         // 0xbd
	{ "CP (HL)", NULL},                      // 0xbe
	{ "CP A", NULL},                         // 0xbf
	{ "RET NZ", NULL},                       // 0xc0
	{ "POP BC", pop_op, &registers.b, &registers.c},                       // 0xc1
	{ "JP NZ, 0x%04X", NULL},                // 0xc2
	{ "JP 0x%04X", jump_operand},                    // 0xc3
	{ "CALL NZ, 0x%04X", NULL},              // 0xc4
	{ "PUSH BC", push_op, &registers.b, &registers.c },                      // 0xc5
	{ "ADD A, 0x%02X", NULL},                // 0xc6
	{ "RST 0x00", NULL},                     // 0xc7
	{ "RET Z", NULL},                        // 0xc8
	{ "RET", NULL},                          // 0xc9
	{ "JP Z, 0x%04X", NULL},                 // 0xca
	{ "CB %02X", execute_cb},                      // 0xcb
	{ "CALL Z, 0x%04X", NULL},               // 0xcc
	{ "CALL 0x%04X", call_operand},                  // 0xcd
	{ "ADC 0x%02X", NULL},                   // 0xce
	{ "RST 0x08", NULL},                     // 0xcf
	{ "RET NC", NULL},                       // 0xd0
	{ "POP DE", NULL},                       // 0xd1
	{ "JP NC, 0x%04X", NULL},                // 0xd2
	{ "UNKNOWN", NULL},                      // 0xd3
	{ "CALL NC, 0x%04X", NULL},              // 0xd4
	{ "PUSH DE", NULL},                      // 0xd5
	{ "SUB 0x%02X", NULL},                   // 0xd6
	{ "RST 0x10", NULL},                     // 0xd7
	{ "RET C", NULL},                        // 0xd8
	{ "RETI", NULL},                         // 0xd9
	{ "JP C, 0x%04X", NULL},                 // 0xda
	{ "UNKNOWN", NULL},                      // 0xdb
	{ "CALL C, 0x%04X", NULL},               // 0xdc
	{ "UNKNOWN", NULL},                      // 0xdd
	{ "SBC 0x%02X", NULL},                   // 0xde
	{ "RST 0x18", NULL},                     // 0xdf
	{ "LD (0xFF00 + 0x%02X), A", load8bit_to_mem_with_offset_operand, &registers.a},      // 0xe0
	{ "POP HL", NULL},                       // 0xe1
	{ "LD (0xFF00 + C), A", load8bit_to_mem_with_offset, &registers.c, &registers.a},           // 0xe2
	{ "UNKNOWN", NULL},                      // 0xe3
	{ "UNKNOWN", NULL},                      // 0xe4
	{ "PUSH HL", NULL},                      // 0xe5
	{ "AND 0x%02X", NULL},                   // 0xe6
	{ "RST 0x20", NULL},                     // 0xe7
	{ "ADD SP,0x%02X", NULL},                // 0xe8
	{ "JP HL", NULL},                        // 0xe9
	{ "LD (0x%04X), A", NULL},               // 0xea
	{ "UNKNOWN", NULL},                      // 0xeb
	{ "UNKNOWN", NULL},                      // 0xec
	{ "UNKNOWN", NULL},                      // 0xed
	{ "XOR 0x%02X", NULL},                   // 0xee
	{ "RST 0x28", NULL},                     // 0xef
	{ "LD A, (0xFF00 + 0x%02X)", NULL},      // 0xf0
	{ "POP AF", NULL},                       // 0xf1
	{ "LD A, (0xFF00 + C)", NULL},           // 0xf2
	{ "DI", NULL},                           // 0xf3
	{ "UNKNOWN", NULL},                      // 0xf4
	{ "PUSH AF", NULL},                      // 0xf5
	{ "OR 0x%02X", NULL},                    // 0xf6
	{ "RST 0x30", NULL},                     // 0xf7
	{ "LD HL, SP+0x%02X", NULL},             // 0xf8
	{ "LD SP, HL", NULL},                    // 0xf9
	{ "LD A, (0x%04X)", NULL},               // 0xfa
	{ "EI", enable_interrupts},                           // 0xfb
	{ "UNKNOWN", NULL},                      // 0xfc
	{ "UNKNOWN", NULL},                      // 0xfd
	{ "CP 0x%02X", cp_operand},                    // 0xfe
	{ "RST 0x38", NULL},                     // 0xff
};


/*
 * Instructions with prefix CB
 */
const struct instruction instructions_cb[256] = {
    { "RLC B", NULL},           // 0x00
	{ "RLC C", NULL},           // 0x01
	{ "RLC D", NULL},           // 0x02
	{ "RLC E", NULL},           // 0x03
	{ "RLC H", NULL},           // 0x04
	{ "RLC L", NULL},           // 0x05
	{ "RLC (HL)", NULL},      // 0x06
	{ "RLC A", NULL},           // 0x07
	{ "RRC B", NULL},           // 0x08
	{ "RRC C", NULL},           // 0x09
	{ "RRC D", NULL},           // 0x0a
	{ "RRC E", NULL},           // 0x0b
	{ "RRC H", NULL},           // 0x0c
	{ "RRC L", NULL},           // 0x0d
	{ "RRC (HL)", NULL},      // 0x0e
	{ "RRC A", NULL},           // 0x0f
	{ "RL B", NULL},             // 0x10
	{ "RL C", rl_op, &registers.c},             // 0x11
	{ "RL D", NULL},             // 0x12
	{ "RL E", NULL},             // 0x13
	{ "RL H", NULL},             // 0x14
	{ "RL L", NULL},             // 0x15
	{ "RL (HL)", NULL},        // 0x16
	{ "RL A", NULL},             // 0x17
	{ "RR B", NULL},             // 0x18
	{ "RR C", NULL},             // 0x19
	{ "RR D", NULL},             // 0x1a
	{ "RR E", NULL},             // 0x1b
	{ "RR H", NULL},             // 0x1c
	{ "RR L", NULL},             // 0x1d
	{ "RR (HL)", NULL},        // 0x1e
	{ "RR A", NULL},             // 0x1f
	{ "SLA B", NULL},           // 0x20
	{ "SLA C", NULL},           // 0x21
	{ "SLA D", NULL},           // 0x22
	{ "SLA E", NULL},           // 0x23
	{ "SLA H", NULL},           // 0x24
	{ "SLA L", NULL},           // 0x25
	{ "SLA (HL)", NULL},      // 0x26
	{ "SLA A", NULL},           // 0x27
	{ "SRA B", NULL},           // 0x28
	{ "SRA C", NULL},           // 0x29
	{ "SRA D", NULL},           // 0x2a
	{ "SRA E", NULL},           // 0x2b
	{ "SRA H", NULL},           // 0x2c
	{ "SRA L", NULL},           // 0x2d
	{ "SRA (HL)", NULL},      // 0x2e
	{ "SRA A", NULL},           // 0x2f
	{ "SWAP B", NULL},         // 0x30
	{ "SWAP C", NULL},         // 0x31
	{ "SWAP D", NULL},         // 0x32
	{ "SWAP E", NULL},         // 0x33
	{ "SWAP H", NULL},         // 0x34
	{ "SWAP L", NULL},         // 0x35
	{ "SWAP (HL)", NULL},    // 0x36
	{ "SWAP A", NULL},         // 0x37
	{ "SRL B", NULL},           // 0x38
	{ "SRL C", NULL},           // 0x39
	{ "SRL D", NULL},           // 0x3a
	{ "SRL E", NULL},           // 0x3b
	{ "SRL H", NULL},           // 0x3c
	{ "SRL L", NULL},           // 0x3d
	{ "SRL (HL)", NULL},      // 0x3e
	{ "SRL A", NULL},           // 0x3f
	{ "BIT 0, B", NULL},      // 0x40
	{ "BIT 0, C", NULL},      // 0x41
	{ "BIT 0, D", NULL},      // 0x42
	{ "BIT 0, E", NULL},      // 0x43
	{ "BIT 0, H", NULL},      // 0x44
	{ "BIT 0, L", NULL},      // 0x45
	{ "BIT 0, (HL)", NULL}, // 0x46
	{ "BIT 0, A", NULL},      // 0x47
	{ "BIT 1, B", NULL},      // 0x48
	{ "BIT 1, C", NULL},      // 0x49
	{ "BIT 1, D", NULL},      // 0x4a
	{ "BIT 1, E", NULL},      // 0x4b
	{ "BIT 1, H", NULL},      // 0x4c
	{ "BIT 1, L", NULL},      // 0x4d
	{ "BIT 1, (HL)", NULL}, // 0x4e
	{ "BIT 1, A", NULL},      // 0x4f
	{ "BIT 2, B", NULL},      // 0x50
	{ "BIT 2, C", NULL},      // 0x51
	{ "BIT 2, D", NULL},      // 0x52
	{ "BIT 2, E", NULL},      // 0x53
	{ "BIT 2, H", NULL},      // 0x54
	{ "BIT 2, L", NULL},      // 0x55
	{ "BIT 2, (HL)", NULL}, // 0x56
	{ "BIT 2, A", NULL},      // 0x57
	{ "BIT 3, B", NULL},      // 0x58
	{ "BIT 3, C", NULL},      // 0x59
	{ "BIT 3, D", NULL},      // 0x5a
	{ "BIT 3, E", NULL},      // 0x5b
	{ "BIT 3, H", NULL},      // 0x5c
	{ "BIT 3, L", NULL},      // 0x5d
	{ "BIT 3, (HL)", NULL}, // 0x5e
	{ "BIT 3, A", NULL},      // 0x5f
	{ "BIT 4, B", NULL},      // 0x60
	{ "BIT 4, C", NULL},      // 0x61
	{ "BIT 4, D", NULL},      // 0x62
	{ "BIT 4, E", NULL},      // 0x63
	{ "BIT 4, H", NULL},      // 0x64
	{ "BIT 4, L", NULL},      // 0x65
	{ "BIT 4, (HL)", NULL}, // 0x66
	{ "BIT 4, A", NULL},      // 0x67
	{ "BIT 5, B", NULL},      // 0x68
	{ "BIT 5, C", NULL},      // 0x69
	{ "BIT 5, D", NULL},      // 0x6a
	{ "BIT 5, E", NULL},      // 0x6b
	{ "BIT 6, H", NULL},      // 0x6c
	{ "BIT 6, L", NULL},      // 0x6d
	{ "BIT 5, (HL)", NULL}, // 0x6e
	{ "BIT 5, A", NULL},      // 0x6f
	{ "BIT 6, B", NULL},      // 0x70
	{ "BIT 6, C", NULL},      // 0x71
	{ "BIT 6, D", NULL},      // 0x72
	{ "BIT 6, E", NULL},      // 0x73
	{ "BIT 6, H", NULL},      // 0x74
	{ "BIT 6, L", NULL},      // 0x75
	{ "BIT 6, (HL)", NULL}, // 0x76
	{ "BIT 6, A", NULL},      // 0x77
	{ "BIT 7, B", NULL},      // 0x78
	{ "BIT 7, C", NULL},      // 0x79
	{ "BIT 7, D", NULL},      // 0x7a
	{ "BIT 7, E", bit_op, (void*) 7, &registers.e },      // 0x7b
	{ "BIT 7, H", bit_op, (void*) 7, &registers.h },      // 0x7c
	{ "BIT 7, L", NULL},      // 0x7d
	{ "BIT 7, (HL)", NULL}, // 0x7e
	{ "BIT 7, A", NULL},      // 0x7f
	{ "RES 0, B", NULL},      // 0x80
	{ "RES 0, C", NULL},      // 0x81
	{ "RES 0, D", NULL},      // 0x82
	{ "RES 0, E", NULL},      // 0x83
	{ "RES 0, H", NULL},      // 0x84
	{ "RES 0, L", NULL},      // 0x85
	{ "RES 0, (HL)", NULL}, // 0x86
	{ "RES 0, A", NULL},      // 0x87
	{ "RES 1, B", NULL},      // 0x88
	{ "RES 1, C", NULL},      // 0x89
	{ "RES 1, D", NULL},      // 0x8a
	{ "RES 1, E", NULL},      // 0x8b
	{ "RES 1, H", NULL},      // 0x8c
	{ "RES 1, L", NULL},      // 0x8d
	{ "RES 1, (HL)", NULL}, // 0x8e
	{ "RES 1, A", NULL},      // 0x8f
	{ "RES 2, B", NULL},      // 0x90
	{ "RES 2, C", NULL},      // 0x91
	{ "RES 2, D", NULL},      // 0x92
	{ "RES 2, E", NULL},      // 0x93
	{ "RES 2, H", NULL},      // 0x94
	{ "RES 2, L", NULL},      // 0x95
	{ "RES 2, (HL)", NULL}, // 0x96
	{ "RES 2, A", NULL},      // 0x97
	{ "RES 3, B", NULL},      // 0x98
	{ "RES 3, C", NULL},      // 0x99
	{ "RES 3, D", NULL},      // 0x9a
	{ "RES 3, E", NULL},      // 0x9b
	{ "RES 3, H", NULL},      // 0x9c
	{ "RES 3, L", NULL},      // 0x9d
	{ "RES 3, (HL)", NULL}, // 0x9e
	{ "RES 3, A", NULL},      // 0x9f
	{ "RES 4, B", NULL},      // 0xa0
	{ "RES 4, C", NULL},      // 0xa1
	{ "RES 4, D", NULL},      // 0xa2
	{ "RES 4, E", NULL},      // 0xa3
	{ "RES 4, H", NULL},      // 0xa4
	{ "RES 4, L", NULL},      // 0xa5
	{ "RES 4, (HL)", NULL}, // 0xa6
	{ "RES 4, A", NULL},      // 0xa7
	{ "RES 5, B", NULL},      // 0xa8
	{ "RES 5, C", NULL},      // 0xa9
	{ "RES 5, D", NULL},      // 0xaa
	{ "RES 5, E", NULL},      // 0xab
	{ "RES 5, H", NULL},      // 0xac
	{ "RES 5, L", NULL},      // 0xad
	{ "RES 5, (HL)", NULL}, // 0xae
	{ "RES 5, A", NULL},      // 0xaf
	{ "RES 6, B", NULL},      // 0xb0
	{ "RES 6, C", NULL},      // 0xb1
	{ "RES 6, D", NULL},      // 0xb2
	{ "RES 6, E", NULL},      // 0xb3
	{ "RES 6, H", NULL},      // 0xb4
	{ "RES 6, L", NULL},      // 0xb5
	{ "RES 6, (HL)", NULL}, // 0xb6
	{ "RES 6, A", NULL},      // 0xb7
	{ "RES 7, B", NULL},      // 0xb8
	{ "RES 7, C", NULL},      // 0xb9
	{ "RES 7, D", NULL},      // 0xba
	{ "RES 7, E", NULL},      // 0xbb
	{ "RES 7, H", NULL},      // 0xbc
	{ "RES 7, L", NULL},      // 0xbd
	{ "RES 7, (HL)", NULL}, // 0xbe
	{ "RES 7, A", NULL},      // 0xbf
	{ "SET 0, B", NULL},      // 0xc0
	{ "SET 0, C", NULL},      // 0xc1
	{ "SET 0, D", NULL},      // 0xc2
	{ "SET 0, E", NULL},      // 0xc3
	{ "SET 0, H", NULL},      // 0xc4
	{ "SET 0, L", NULL},      // 0xc5
	{ "SET 0, (HL)", NULL}, // 0xc6
	{ "SET 0, A", NULL},      // 0xc7
	{ "SET 1, B", NULL},      // 0xc8
	{ "SET 1, C", NULL},      // 0xc9
	{ "SET 1, D", NULL},      // 0xca
	{ "SET 1, E", NULL},      // 0xcb
	{ "SET 1, H", NULL},      // 0xcc
	{ "SET 1, L", NULL},      // 0xcd
	{ "SET 1, (HL)", NULL}, // 0xce
	{ "SET 1, A", NULL},      // 0xcf
	{ "SET 2, B", NULL},      // 0xd0
	{ "SET 2, C", NULL},      // 0xd1
	{ "SET 2, D", NULL},      // 0xd2
	{ "SET 2, E", NULL},      // 0xd3
	{ "SET 2, H", NULL},      // 0xd4
	{ "SET 2, L", NULL},      // 0xd5
	{ "SET 2, (HL)", NULL}, // 0xd6
	{ "SET 2, A", NULL},      // 0xd7
	{ "SET 3, B", NULL},      // 0xd8
	{ "SET 3, C", NULL},      // 0xd9
	{ "SET 3, D", NULL},      // 0xda
	{ "SET 3, E", NULL},      // 0xdb
	{ "SET 3, H", NULL},      // 0xdc
	{ "SET 3, L", NULL},      // 0xdd
	{ "SET 3, (HL)", NULL}, // 0xde
	{ "SET 3, A", NULL},      // 0xdf
	{ "SET 4, B", NULL},      // 0xe0
	{ "SET 4, C", NULL},      // 0xe1
	{ "SET 4, D", NULL},      // 0xe2
	{ "SET 4, E", NULL},      // 0xe3
	{ "SET 4, H", NULL},      // 0xe4
	{ "SET 4, L", NULL},      // 0xe5
	{ "SET 4, (HL)", NULL}, // 0xe6
	{ "SET 4, A", NULL},      // 0xe7
	{ "SET 5, B", NULL},      // 0xe8
	{ "SET 5, C", NULL},      // 0xe9
	{ "SET 5, D", NULL},      // 0xea
	{ "SET 5, E", NULL},      // 0xeb
	{ "SET 5, H", NULL},      // 0xec
	{ "SET 5, L", NULL},      // 0xed
	{ "SET 5, (HL)", NULL}, // 0xee
	{ "SET 5, A", NULL},      // 0xef
	{ "SET 6, B", NULL},      // 0xf0
	{ "SET 6, C", NULL},      // 0xf1
	{ "SET 6, D", NULL},      // 0xf2
	{ "SET 6, E", NULL},      // 0xf3
	{ "SET 6, H", NULL},      // 0xf4
	{ "SET 6, L", NULL},      // 0xf5
	{ "SET 6, (HL)", NULL}, // 0xf6
	{ "SET 6, A", NULL},      // 0xf7
	{ "SET 7, B", NULL},      // 0xf8
	{ "SET 7, C", NULL},      // 0xf9
	{ "SET 7, D", NULL},      // 0xfa
	{ "SET 7, E", NULL},      // 0xfb
	{ "SET 7, H", NULL},      // 0xfc
	{ "SET 7, L", NULL},      // 0xfd
	{ "SET 7, (HL)", set_op_from_mem, (void*) 7, &registers.hl}, // 0xfe
	{ "SET 7, A", NULL},      // 0xff
};

void execute_cb() {
    
    unsigned char opcode = memory[registers.pc++];
    struct instruction instruction = instructions_cb[opcode];
    if (instruction.execute) {

        instruction.execute(instruction.exec_argv1, instruction.exec_argv2);

    } else {

        printf("CB Operation not defined: %s -> 0x%x\n", instruction.disassembly, opcode);
        exit(1);

    }

}


void execute() {

    unsigned char opcode = memory[registers.pc++];
    struct instruction instruction = instructions[opcode];
    if (instruction.execute) {

        printf("%s -> 0x%x\n", instruction.disassembly, opcode);
        instruction.execute(instruction.exec_argv1, instruction.exec_argv2);

        debug();

    } else {

        printf("Operation not defined: %s -> 0x%x\n", instruction.disassembly, opcode);
        exit(1);

    }

}

void boot() {

    debug();
    while (1) {
        execute();
    }

    // TODO: set boot rom, set flags

}
