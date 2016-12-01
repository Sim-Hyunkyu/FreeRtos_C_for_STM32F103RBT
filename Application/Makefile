###############################################################################
#
#  	    File        : Makefile
#
#       Abstract    : Example Makefile for a C++ Project
#
#       Environment : Atollic TrueSTUDIO(R)
#
###############################################################################

SHELL=cmd

# System configuration
CC = arm-atollic-eabi-gcc
CXX = arm-atollic-eabi-g++
RM=rm -rf

# Assembler, Compiler and Linker flags and linker script settings
LINKER_FLAGS=-lm -mthumb -mcpu=cortex-m3  -Wl,--gc-sections -T$(LINK_SCRIPT) -static  -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group -specs=nano.specs -specs=nosys.specs  -Wl,-cref "-Wl,-Map=$(BIN_DIR)/Application.map" -Wl,--defsym=malloc_getpagesize_P=0x1000
LINK_SCRIPT="stm32_flash.ld"
ASSEMBLER_FLAGS=-c -g -O0 -mcpu=cortex-m3  -mthumb  -x assembler-with-cpp  -IUtilities\STM32F103_Nucleo -Isrc -ILibraries\STM32F10x_StdPeriph_Driver\inc -ILibraries\CMSIS\Device\ST\STM32F10x\Include -ILibraries\CMSIS\Include
COMPILER_FLAGS=-c -g -mcpu=cortex-m3  -O0 -Wall -ffunction-sections -fdata-sections -mthumb -D"USE_NUCLEO_F103RB" -D"HSI_VALUE=8000000" -D"STM32F10X_MD" -D"USE_STDPERIPH_DRIVER"   -IUtilities\STM32F103_Nucleo -Isrc -ILibraries\STM32F10x_StdPeriph_Driver\inc -ILibraries\CMSIS\Device\ST\STM32F10x\Include -ILibraries\CMSIS\Include
CXXCOMPILER_FLAGS=-fno-threadsafe-statics -c -g -mcpu=cortex-m3  -O0 -Wall -ffunction-sections -fdata-sections -mthumb -fno-exceptions -fno-rtti -D"USE_NUCLEO_F103RB" -D"HSI_VALUE=8000000" -D"STM32F10X_MD" -D"USE_STDPERIPH_DRIVER"   -IUtilities\STM32F103_Nucleo -Isrc -ILibraries\STM32F10x_StdPeriph_Driver\inc -ILibraries\CMSIS\Device\ST\STM32F10x\Include -ILibraries\CMSIS\Include

# Define output directory
OBJECT_DIR = Debug
BIN_DIR = $(OBJECT_DIR)

# Define sources and objects
SRC := $(wildcard */*/*/*/*/*/*/*.c) \
	$(wildcard */*/*/*/*/*/*.c) \
	$(wildcard */*/*/*/*/*.c) \
	$(wildcard */*/*/*/*.c) \
	$(wildcard */*/*/*.c) \
	$(wildcard */*/*.c) \
	$(wildcard */*.c) \
	$(wildcard */*/*/*/*/*/*/*.cpp) \
	$(wildcard */*/*/*/*/*/*.cpp) \
	$(wildcard */*/*/*/*/*.cpp) \
	$(wildcard */*/*/*/*.cpp) \
	$(wildcard */*/*/*.cpp) \
	$(wildcard */*/*.cpp) \
	$(wildcard */*.cpp)
SRCSASM := $(wildcard */*/*/*/*/*/*/*/*.s) \
	$(wildcard */*/*/*/*/*/*/*.s) \
	$(wildcard */*/*/*/*/*/*.s) \
	$(wildcard */*/*/*/*/*.s) \
	$(wildcard */*/*/*/*.s) \
	$(wildcard */*/*/*.s) \
	$(wildcard */*/*.s) \
	$(wildcard */*.s)
OBJS := $(SRC:%.c=$(OBJECT_DIR)/%.o) $(SRCSASM:%.s=$(OBJECT_DIR)/%.o)
OBJS := $(OBJS:%.S=$(OBJECT_DIR)/%.o)  
OBJS := $(OBJS:%.cpp=$(OBJECT_DIR)/%.o)


###############
# Build project
# Major targets
###############
all: buildelf

buildelf: $(OBJS) 
	$(CXX) -o "$(BIN_DIR)/Application.elf" $(OBJS) $(LINKER_FLAGS)

clean:
	$(RM) $(OBJS) "$(BIN_DIR)/Application.elf" "$(BIN_DIR)/Application.map"


##################
# Specific targets
##################
$(OBJECT_DIR)/src/main.o: src/main.cpp
	@mkdir $(subst /,\,$(dir $@)) 2> NUL || echo off
	$(CXX) $(CXXCOMPILER_FLAGS) src/main.cpp -o $(OBJECT_DIR)/src/main.o 


##################
# Implicit targets
##################
$(OBJECT_DIR)/%.o: %.c
	@mkdir $(subst /,\,$(dir $@)) 2> NUL || echo off
	$(CC) $(COMPILER_FLAGS) $< -o $@

$(OBJECT_DIR)/%.o: %.cpp
	@mkdir $(subst /,\,$(dir $@)) 2> NUL || echo off
	$(CXX) $(CXXCOMPILER_FLAGS) $< -o $@

$(OBJECT_DIR)/%.o: %.s
	@mkdir $(subst /,\,$(dir $@)) 2> NUL || echo off
	$(CC) $(ASSEMBLER_FLAGS) $< -o $@
	
$(OBJECT_DIR)/%.o: %.S
	@mkdir $(subst /,\,$(dir $@)) 2> NUL || echo off
	$(CC) $(ASSEMBLER_FLAGS) $< -o $@