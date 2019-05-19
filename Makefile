# If cross compiling from windows use native GNU-Make 4.2.1
# https://sourceforge.net/projects/ezwinports/files/
# download "make-4.2.1-without-guile-w32-bin.zip" and set it on the enviroment path
# There is no need to install cygwin or any of that sort of rubbish

# This just defines a couple of command that differ from windows to linux used in clean
ifeq ($(OS), Windows_NT)
	#WINDOWS USE THESE DEFINITIONS
	RM = -del /q
	SLASH = \\
else
	#LINUX USE THESE DEFINITIONS
	RM = -rm -f
	SLASH = /
endif 

# This is your executables for compiling, linking, assembler etc
CC       = xc16-gcc
LD       = xc16-gcc
AS       = xc16-as
AR       = xc16-ar
OBJCOPY  = xc16-objcopy
STRIP    = xc16-strip

# These are you compiler flags
CFLAGS = -O1 -fno-strict-aliasing -Wall -ffunction-sections -mcpu=24FJ128GA010 

# These are your linker flags
LDFLAGS = -mcpu=24FJ128GA010 -T p24FJ128GA010.gld -Wl,-Map=contiki-explorer16.map -s
LDFLAGS += -Wl,--defsym,_min_stack_size=4096 -Wl,--gc-sections

# The directory from the makefile directory in which sets of C source files are stored add as many as you like
SOURCE_C1 = ${CURDIR}/$(CONTIKI)/cpu/pic24f
SOURCE_C2 = ${CURDIR}/$(CONTIKI)/core/

# The directory from the makefile directory in which sets of asm .S or .s source files are stored add as many as you like
SOURCE_A1 = ${CURDIR}/$(CONTIKI)/cpu/pic24f

# The directory in which the build files are compiled too (.o and .so files) .. make sure it exists
BUILD = ${CURDIR}/Build

# This creates the names of all .S and .s files that must be compiled from your assembler directories above
ASMOBJS = $(patsubst $(SOURCE_A1)/%.S,$(BUILD)/%.o,$(wildcard $(SOURCE_A1)/*.S))
ASMOBJS += $(patsubst $(SOURCE_A1)/%.s,$(BUILD)/%.o,$(wildcard $(SOURCE_A1)/*.s))

# The names of all C files that must be compiled from the source directories above
COBJS = $(patsubst $(SOURCE_C1)/%.c,$(BUILD)/%.o,$(wildcard $(SOURCE_C1)/*.c))
COBJS += $(patsubst $(SOURCE_C2)/%.c,$(BUILD)/%.o,$(wildcard $(SOURCE_C2)/*.c))

#We are going to build the elf file contiki.elf
all: contiki.elf

#Rule 1 assemble any .s files to .o in build directory add any extra source directories like first
$(BUILD)/%.o: $(SOURCE_A1)/%.s
	$(AS) -MMD -MP -g $(CFLAGS) -c  $< -o $@

#Rule 2 assemble any .S files to .o in build directory add any extra source directories like first
$(BUILD)/%.o: $(SOURCE_A1)/%.S
	$(AS) -MMD -MP -g $(CFLAGS) -c  $< -o $@

#Rule 3 assemble any .c files to .o in build directory add any extra source directories
$(BUILD)/%.o: $(SOURCE_C1)/%.c $(SOURCE_C2)/%.c
	$(CC) -MMD -MP -g $(CFLAGS) -c  $< -o $@

#To build contiki.elf we must compile all the .S, .S and .C files in the source directories 
# the elf is then converted to yoru binary if you want
contiki.elf: $(ASMOBJS) $(COBJS) 
	$(LD) $(LDFLAGS) $(ASMOBJS) $(COBJS) -o contiki.elf
	$(OBJCOPY) contiki.elf -O binary whatever_name_you_want.bin

# Control silent mode  .... we want silent in clean
.SILENT: clean

# cleanup temp files in the build directory
clean:
	$(RM) $(BUILD)$(SLASH)*.o 
	$(RM) $(BUILD)$(SLASH)*.d 
	echo CLEAN COMPLETED
.PHONY: clean

