#ifndef _CPU

#define _CPU
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

extern struct registers registers;


/*---- Flags ------------------------------------------------------*/


#define FLAG_Z ((unsigned char) 128) // 1000 0000
#define FLAG_N ((unsigned char) 64) // 0100 0000
#define FLAG_H ((unsigned char) 32) // 0010 0000
#define FLAG_CY ((unsigned char) 16) // 0001 0000



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


/*---- Interrupts -------------------------------------------------*/


#define VBLANK_INTERRUPT ((unsigned char) 1) // 0000 0001
#define LCDSTAT_INTERRUPT ((unsigned char) 2) // 0000 0010
#define TIMER_INTERRUPT ((unsigned char) 4) // 0000 0100
#define SERIAL_INTERRUPT ((unsigned char) 8) // 0000 1000
#define JOYPAD_INTERRUPT ((unsigned char) 16) // 0001 0000

void request_interrupt(unsigned char interrupt_flag);



/*---- Main Logic and Execution -----------------------------------*/

int cpu();
void boot_tests();

#endif
