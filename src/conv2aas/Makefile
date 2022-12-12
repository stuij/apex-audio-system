# Payback makefile

# (or rather the conv2aas makefile, but the above line is from
# the original author and as Payback was the game this code was made for, I've
# got a hard time deleting this historic remnant)

# Tools and flags.
HOST_PREFIX ?=
HOST_CC = $(HOST_PREFIX)gcc
CFLAGS = -O3

HOST_LD = $(HOST_PREFIX)gcc
LDFLAGS =

# Name of output targets.
TARGET = conv2aas
ifeq ($(OS),Windows_NT)
	TARGET = conv2aas.exe
endif

# C source files.
SRC =	Main.o

# Here follow the generic build rules.
all: $(TARGET)

release:
	make clean all
	rm -f *.o $(GFX) $(SOUND) $(SYMBOLS) $(MAP)

clean:
	rm -f *.o $(TARGET) $(GFX) $(SOUND) $(SYMBOLS) $(MAP)

%.o: %.c
	$(HOST_CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(TARGET): $(SRC)
	$(HOST_LD) $(LDFLAGS) -o $@ $(SRC) $(LIBS)
	$(HOST_PREFIX)size $@
