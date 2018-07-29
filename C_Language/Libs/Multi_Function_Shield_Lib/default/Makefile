###############################################################################
# Makefile for the project Multi_Function_Shield_Lib
###############################################################################

## General Flags
PROJECT = Multi_Function_Shield_Lib
MCU = atmega8
TARGET = Multi_Function_Shield_Lib.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99           -DF_CPU=8000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=Multi_Function_Shield_Lib.map


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Include Directories
INCLUDES = -I"D:\Docs\Dropbox\Embed\AVR\Libs\C\Multi_Function_Shield_Lib\." 

## Objects that must be built in order to link
OBJECTS = Multi_Function_Shield_Lib.o Main.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) Multi_Function_Shield_Lib.hex Multi_Function_Shield_Lib.eep Multi_Function_Shield_Lib.lss size

## Compile
Multi_Function_Shield_Lib.o: ../Multi_Function_Shield_Lib.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

Main.o: ../Main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) Multi_Function_Shield_Lib.elf dep/* Multi_Function_Shield_Lib.hex Multi_Function_Shield_Lib.eep Multi_Function_Shield_Lib.lss Multi_Function_Shield_Lib.map


## Other dependencies
-include $(shell mkdir dep 2>NUL) $(wildcard dep/*)

