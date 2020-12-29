emulator: main.c cpu.c memory.c gpu.c
	gcc -Wall -o emulator main.c

clean:
	rm emulator

run:
	./emulator
