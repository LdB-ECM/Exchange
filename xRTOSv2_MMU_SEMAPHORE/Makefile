# If cross compiling from windows use native GNU-Make 4.2.1
# https://sourceforge.net/projects/ezwinports/files/
# download "make-4.2.1-without-guile-w32-bin.zip" and set it on the enviroment path
# There is no need to install cygwin or any of that sort of rubbish

ifeq ($(OS), Windows_NT)
	#WINDOWS USE THESE DEFINITIONS
	RM = -del /q
	SLASH = \\
else
	#LINUX USE THESE DEFINITIONS
	RM = -rm -f
	SLASH = /
endif 

ifeq ($(MAKECMDGOALS),Pi1)
$(error This is a multicore project Pi1 is not a valid target being single core)
exit
endif

# Platform to build ... Pi3-64
ifeq ($(MAKECMDGOALS),Pi3-64)
$(error This platform conversion is not yet complete )
exit
endif


# Platform to build ... Pi2
ifeq ($(MAKECMDGOALS),Pi2)
	PLATFORM := RaspberryPi32
	ARCH_ROOT := D:/GCC/gcc-arm-none-eabi-8/bin/
	ARCH_PREFIX := arm-none-eabi
	CODE_ARCH := arm7 aarch32
	CFLAGS   := -O3 -mcpu=cortex-a7 -mfpu=neon -mfloat-abi=hard -mno-unaligned-access
	LINKERFILE := rpi32.ld
	IMAGEFILE := diskimg/kerne7.img
endif

# Platform to build ... Pi3
ifeq ($(MAKECMDGOALS),Pi3)
	PLATFORM := RaspberryPi32
	ARCH_ROOT := D:/GCC/gcc-arm-none-eabi-8/bin/
	ARCH_PREFIX := arm-none-eabi
	CODE_ARCH := arm8 aarch32
	CFLAGS   := -O3 -mcpu=cortex-a53 -mfpu=neon-vfpv4 -mfloat-abi=hard -mno-unaligned-access
	LINKERFILE := rpi32.ld
	IMAGEFILE := diskimg/kernel8-32.img
endif

# Platform to build ... Pi3-64
ifeq ($(MAKECMDGOALS),Pi3-64)
	PLATFORM := RaspberryPi64
	ARCH_ROOT := D:/GCC/gcc-linaro-7.5.0-2019.12-i686-mingw32_aarch64-elf/bin/
	ARCH_PREFIX := aarch64-elf
	CODE_ARCH := arm8 aarch64
	CFLAGS   := -O3 -mcpu=cortex-a53+fp+simd -mstrict-align
	LINKERFILE := rpi64.ld
	IMAGEFILE := diskimg/kernel8.img
endif

# Add to all builds
CFLAGS += -Wall -std=c11 -ffreestanding -nostartfiles -Wno-nonnull-compare -fno-tree-loop-vectorize -fno-tree-slp-vectorize

# The directory in which source files are stored.
SOURCE = ${CURDIR}
BUILD = Build

#Create full compiler path
ARMGNU := $(ARCH_ROOT)$(ARCH_PREFIX)

#Create platform  path
PLATFORM_DIR := $(SOURCE)/platform/$(PLATFORM)


# The name of the assembler listing file to generate.
LIST = kernel.list

# The name of the map file to generate.
MAP = kernel.map

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.

ASMOBJS = $(patsubst $(PLATFORM_DIR)/%.S,$(BUILD)/%.o,$(wildcard $(PLATFORM_DIR)/*.S))
COBJS = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(wildcard $(SOURCE)/*.c))  $(patsubst $(PLATFORM_DIR)/%.c,$(BUILD)/%.o,$(wildcard $(PLATFORM_DIR)/*.c))


Pi3-64 Pi3 Pi2 Pi1: kernel.elf
.PHONY: kernel.elf

$(BUILD)/%.o: $(PLATFORM_DIR)/%.S
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ 

$(BUILD)/%.o: $(PLATFORM_DIR)/%.c
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ 

$(BUILD)/%.o: $(SOURCE)/%.c
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ 

kernel.elf: $(ASMOBJS) $(COBJS) 
	$(ARMGNU)-gcc $(CFLAGS) $(ASMOBJS) $(COBJS) -T$(PLATFORM_DIR)/$(LINKERFILE) -Wl,--build-id=none -o kernel.elf -lc -lm -lgcc
	$(ARMGNU)-objdump -d kernel.elf > $(LIST)
	$(ARMGNU)-objcopy kernel.elf -O binary $(IMAGEFILE)
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

