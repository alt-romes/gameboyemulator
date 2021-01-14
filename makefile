emulator: emulator.c cpu.c memory.c ppu.c
	gcc -Wall -o emulator emulator.c -framework OpenGL -lglew -lGLFW

clean:
	rm emulator

run:
	./emulator

debug:
	./emulator -d

test:
	./emulator -t tests/cpu_instrs/cpu_instrs.gb

t1:
	./emulator -t tests/cpu_instrs/individual/01-special.gb
t2:
	./emulator -t tests/cpu_instrs/individual/02-interrupts.gb
t3:
	./emulator -t tests/cpu_instrs/individual/03-op sp,hl.gb
t4:
	./emulator -t tests/cpu_instrs/individual/04-op r,imm.gb
t5:
	./emulator -t tests/cpu_instrs/individual/05-op rp.gb
t6:
	./emulator -t tests/cpu_instrs/individual/06-ld r,r.gb
t7:
	./emulator -t tests/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb
t8:
	./emulator -t tests/cpu_instrs/individual/08-misc instrs.gb
t9:
	./emulator -t tests/cpu_instrs/individual/09-op r,r.gb
t10:
	./emulator -t tests/cpu_instrs/individual/10-bit ops.gb
t11:
	./emulator -t tests/cpu_instrs/individual/11-op a,\(hl\).gb
