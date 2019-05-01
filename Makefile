# If cross compiling from windows use native GNU-Make 4.2.1
# https://sourceforge.net/projects/ezwinports/files/
# download "make-4.2.1-without-guile-w32-bin.zip" and set it on the enviroment path
# There is no need to install cygwin or any of that sort of rubbish

ifeq ($(OS), Windows_NT)
	#WINDOWS USE THESE DEFINITIONS
	RM = -del /q /f
	SLASH = \\
	SWALLOW_OUTPUT = >nul 2>nul
else
	#LINUX USE THESE DEFINITIONS
	RM = -rm -f
	SLASH = /
	SWALLOW_OUTPUT =
endif 


CFLAGS = -Wall -O3 -mcpu=cortex-a53+fp+simd -ffreestanding -nostartfiles -std=c11 -mstrict-align -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wno-nonnull-compare
ARMGNU = aarch64-elf
LINKERFILE = rpi64.ld
SMARTSTART = SmartStart64.S
IMGFILE = kernel8.img


# The directory in which source files are stored.
SOURCE = ${CURDIR}
BUILD = Build


# The name of the assembler listing file to generate.
LIST = kernel.list

# The name of the map file to generate.
MAP = kernel.map

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.

ASMOBJS = $(SOURCE)/$(SMARTSTART)
COBJS = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(wildcard $(SOURCE)/*.c))

BINARY = $(IMGFILE)

all: kernel.elf

$(BUILD)/%.o: $(SOURCE)/%.s
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

$(BUILD)/%.o: $(SOURCE)/%.S
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

$(BUILD)/%.o: $(SOURCE)/%.c
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

kernel.elf: $(ASMOBJS) $(COBJS) 
	$(ARMGNU)-gcc $(CFLAGS) $(ASMOBJS) $(COBJS) -T $(LINKERFILE) -o kernel.elf -lc -lm -lgcc
	$(ARMGNU)-objdump -d kernel.elf > $(LIST)
	$(ARMGNU)-objcopy kernel.elf -O binary DiskImg/$(BINARY)
	$(ARMGNU)-nm -n kernel.elf > $(MAP)

# Control silent mode  .... we want silent in clean
.SILENT: clean

# cleanup temp files
clean:
	$(RM) $(MAP) 
	$(RM) kernel.elf 
	$(RM) $(LIST) 
	$(RM) $(BUILD)$(SLASH)*.o 
	$(RM) $(BUILD)$(SLASH)*.d 
	echo CLEAN COMPLETED
.PHONY: clean

