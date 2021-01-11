# compiler tools prefix used by makefiles for gba executables
# for example, to set to absolute devkitpro path, set to:
# /opt/devkitpro/devkitARM/bin/arm-none-eabi-
export CROSS = arm-none-eabi-

# compiler tools prefix used by makefiles for conv2aas executable
# for example, the empty prefix below will use the system gcc tools
export PREFIX = 

# example names
export EXAMPLE_PREFIX=AASExample
export EXAMPLE2_PREFIX=AASExample2
export EXAMPLE_CPP_PREFIX=AASExamplePlusPlus

BUILD = build

AAS_DIR = src/aas
CONV2AAS_DIR = src/conv2aas
EXAMPLE_DIR = examples/AASExample
EXAMPLE2_DIR = examples/AASExample2
EXAMPLE_CPP_DIR = examples/AASExample_c++

CONV2AAS_FILE = conv2aas
ifeq ($(OS),Windows_NT)
	CONV2AAS_FILE = conv2aas.exe
endif

LIB = $(BUILD)/aas/lib
INCLUDE = $(BUILD)/aas/include
BUILD_AAS = $(LIB)/libAAS.a
BUILD_CONV2AAS_DIR = $(BUILD)/conv2aas
BUILD_CONV2AAS = $(BUILD_CONV2AAS_DIR)/$(CONV2AAS_FILE)
BUILD_EXAMPLES = $(BUILD)/examples

AAS = $(AAS_DIR)/libAAS.a
CONV2AAS = $(CONV2AAS_DIR)/$(CONV2AAS_FILE)
EXAMPLE = $(EXAMPLE_DIR)/$(EXAMPLE_PREFIX).gba
EXAMPLE2 = $(EXAMPLE2_DIR)/$(EXAMPLE2_PREFIX).gba
EXAMPLE_CPP = $(EXAMPLE_CPP_DIR)/$(EXAMPLE_CPP_PREFIX).gba

.PHONY: all aas conv2aas example example2 example_cpp

all: aas conv2aas example example2 example_cpp

$(AAS):
	$(MAKE) -e -C $(AAS_DIR)

$(BUILD_AAS): $(AAS)
	mkdir -p $(LIB)
	mkdir -p $(INCLUDE)
	cp $(AAS) $(LIB)
	cp $(AAS_DIR)/AAS.h $(INCLUDE)
	cp $(AAS_DIR)/AAS_Mixer.h $(INCLUDE)

aas: $(BUILD_AAS)

$(CONV2AAS):
	$(MAKE) -e -C $(CONV2AAS_DIR)

$(BUILD_CONV2AAS): $(CONV2AAS)
	mkdir -p $(BUILD_CONV2AAS_DIR)
	cp $(CONV2AAS) $(BUILD_CONV2AAS_DIR)

conv2aas: $(BUILD_CONV2AAS)

$(EXAMPLE): $(BUILD_AAS) $(BUILD_CONV2AAS)
	$(MAKE) -e -C $(EXAMPLE_DIR)

example: $(EXAMPLE)
	mkdir -p $(BUILD_EXAMPLES)
	cp $(EXAMPLE) $(BUILD_EXAMPLES)

$(EXAMPLE2): $(BUILD_AAS) $(BUILD_CONV2AAS)
	$(MAKE) -e -C $(EXAMPLE2_DIR)

example2: $(EXAMPLE2)
	mkdir -p $(BUILD_EXAMPLES)
	cp $(EXAMPLE2) $(BUILD_EXAMPLES)

$(EXAMPLE_CPP): $(BUILD_AAS) $(BUILD_CONV2AAS)
	$(MAKE) -e -C $(EXAMPLE_CPP_DIR)

example_cpp: $(EXAMPLE_CPP)
	mkdir -p $(BUILD_EXAMPLES)
	cp $(EXAMPLE_CPP) $(BUILD_EXAMPLES)

clean:
	$(MAKE) -C $(AAS_DIR) clean
	$(MAKE) -C $(CONV2AAS_DIR) clean
	$(MAKE) -C $(EXAMPLE_DIR) clean
	$(MAKE) -C $(EXAMPLE2_DIR) clean
	$(MAKE) -C $(EXAMPLE_CPP_DIR) clean
	rm -r build
