DEBUG=0
DEBUGT=256
TEST=0
TESTPATH=""

ifeq ($(TEST), 0)
	TESTPATH = "tests/cpu_instrs/cpu_instrs.gb"
endif
ifeq ($(TEST), 1)
	TESTPATH = "tests/cpu_instrs/individual/01-special.gb"
endif
ifeq ($(TEST), 2)
	TESTPATH = "tests/cpu_instrs/individual/02-interrupts.gb"
endif
ifeq ($(TEST), 3)
	TESTPATH = "tests/cpu_instrs/individual/03-op sp,hl.gb"
endif
ifeq ($(TEST), 4)
	TESTPATH = "tests/cpu_instrs/individual/04-op r,imm.gb"
endif
ifeq ($(TEST), 5)
	TESTPATH = "tests/cpu_instrs/individual/05-op rp.gb"
endif
ifeq ($(TEST), 6)
	TESTPATH = "tests/cpu_instrs/individual/06-ld r,r.gb"
endif
ifeq ($(TEST), 7)
	TESTPATH = "tests/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb"
endif
ifeq ($(TEST), 8)
	TESTPATH = "tests/cpu_instrs/individual/08-misc instrs.gb"
endif
ifeq ($(TEST), 9)
	TESTPATH = "tests/cpu_instrs/individual/09-op r,r.gb"
endif
ifeq ($(TEST), 10)
	TESTPATH = "tests/cpu_instrs/individual/10-bit ops.gb"
endif
ifeq ($(TEST), 11)
	TESTPATH = "tests/cpu_instrs/individual/11-op a,(hl).gb"
endif

emulator: emulator.c cpu.c memory.c ppu.c
	gcc -Wall -o emulator emulator.c -framework OpenGL -lglew -lGLFW

clean:
	rm emulator

run: emulator
	./emulator

debug:
	./emulator -d $(DEBUG)

test:
	./emulator -t $(TESTPATH)

dt:
	./emulator -t $(TESTPATH) -d $(DEBUGT)

