# AAS Example Makefile
ifdef COMPILE_WITH_LLVM
	ifeq ($(strip $(GBA_LLVM)),)
		$(error Please set GBA_LLVM in your environment. export GBA_LLVM=<path to gba-llvm installation>)
	endif

	LD = $(GBA_LLVM)/bin/clang
	OBJCOPY = $(GBA_LLVM)/bin/llvm-objcopy
	SIZE = $(GBA_LLVM)/bin/llvm-size
else
	CROSS ?= arm-none-eabi-
	LD = $(CROSS)gcc
	OBJCOPY = $(CROSS)objcopy
	SIZE = $(CROSS)size
endif

EXAMPLE2_PREFIX ?= AASExample2
SHORTNAME = $(EXAMPLE2_PREFIX)

TOOLS ?= 
PREFIX ?= 

include ../../make/example.make
