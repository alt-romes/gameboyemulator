emulator: emulator.c cpu.c memory.c ppu.c
	gcc -Wall -o emulator emulator.c -framework OpenGL -lglew -lGLFW

clean:
	rm emulator

run:
	./emulator
