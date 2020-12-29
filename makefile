emulator: emulator.c cpu.c memory.c ppu.c
	gcc -Wall -o emulator emulator.c

clean:
	rm emulator

run:
	./emulator
