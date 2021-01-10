# Library and include paths.
LIBS = -L../../build/aas/lib -lAAS
INCLUDES = -I../../build/aas/include

# Files you want to go in ROM (AAS_Data.o must go first)
SRC =	AAS_Data.o AASExample.o

MAP = map.out
LDFLAGS = -mthumb-interwork -Xlinker -Map $(MAP) -nostartfiles -Tlnkscript

# Name of output targets.
NAME = $(SHORTNAME).tmp
TARGET = $(SHORTNAME).gba

# Files you want to go in IWRAM.
IWRAM =	

include ../../make/common.make

AAS_Data.o:
	../../build/conv2aas/conv2aas AAS_Data
	$(AS) $(ASFLAGS) -o $@ AAS_Data.s

$(NAME): crt0.o $(IWRAM) $(GFX) $(SOUND) $(SRC)
	touch $(MAP)
	$(LD) $(LDFLAGS) -o $@ crt0.o $(SRC) $(IWRAM) $(GFX) $(SOUND) $(LIBS)
	$(CROSS)size $@

$(TARGET): $(NAME)
	$(CROSS)objcopy -v -O binary $< $@
	gbafix $@

clean:
	rm -f *.o AAS_Data.h AAS_Data.s $(TARGET) $(NAME) $(GFX) $(SOUND) $(SYMBOLS) $(MAP)
