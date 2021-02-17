CC := gcc
CFLAGS := -Wall -g -Werror=missing-declarations -Werror=redundant-decls
LFLAGS := -framework OpenGL -lglew -lGLFW

 # Include directory
IDIR := include
# Source directory
SDIR = src
# Object directory
ODIR := obj

INCLUDES = -I$(IDIR)

# $(wildcard pattern…) - this string is replaced by a space-separated list
# 	of names of existing files that match one of the given file name patterns.
#
# List of header (dependencies) files in $(IDIR) (include directory)
DEPENDENCIES = $(wildcard $(IDIR)/*.h)
# $(patsubst pattern,replacement,text) - finds whitespace-separated words in text
# 	that match pattern and replaces them with replacement. 
#
# List of object files needed to produce the executable - this is will be
# 	used to say all .c files must be compiled into .o objects
OBJECTS = $(patsubst %.c,%.o,$(wildcard $(SDIR)/*.c))


# Specifying objects as a dependency makes the compiler first compile the individual c files into objects, and only then build the executable
# ($<) is the first item in the dependencies list, ($@) is the left side of the rule's "":"" 
#
# Rule to describe how all .c files must be compiled into .o files with the needed dependencies
%.o: %.c $(DEPENDENCIES)
	$(CC) $(INCLUDES) -c $< -o $@ $(CFLAGS)

# ($^) is the right side of the rule's "":""
# So this rule will put all .o files together to output the executable named "emulator"
#
# Rule to build the executable named "emulator"
emulator: $(OBJECTS)
	$(CC) $(INCLUDES) $^ -o $@ $(CFLAGS) $(LFLAGS)
	@echo All complete!


DEBUG=0
DEBUGT=256

debug: emulator
	./emulator -d $(DEBUG)

test: emulator
	./emulator -t $(TESTPATH)

dt: emulator
	./emulator -t $(TESTPATH) -d $(DEBUGT)

clean:
	rm $(ODIR)/*.o
	rm emulator

run: emulator
	./emulator

