ifeq ($(OS), Windows_NT)
	#WINDOWS USE THESE DEFINITIONS
	RM = -del /q 
	CP = copy
	SLASH = \\
else
	#LINUX USE THESE DEFINITIONS
	RM = rm -f
	CP = mv
	SLASH = /
endif 

COMPILER_ROOT = G:\Pi\gcc_linaro_7_1\bin\aarch64-elf-

SOURCE = src/
NONOPTIMISED = src/NoOptimisation/
BUILD = build/
TARGET = kernel8.img
LINKER = rpi64.ld
FONT = src/font/font.psf
FONTOBJ = build/font.o
AUDIO = src/audio/Interlude.bin
AUDIOOBJ = build/Interlude.o
ASMOBJS = $(patsubst $(SOURCE)%.S,$(BUILD)%.o,$(wildcard $(SOURCE)*.S)) $(patsubst $(SOURCE)%.s,$(BUILD)%.o,$(wildcard $(SOURCE)*.s))
COBJS = $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))
COBJSNOOP = $(patsubst $(NONOPTIMISED)%.c,$(BUILD)%.o,$(wildcard $(NONOPTIMISED)*.c))

CFLAGS = -Wall -O2 -mcpu=cortex-a53 -ffreestanding -nostartfiles -std=c11 -mstrict-align -fno-tree-loop-vectorize -fno-tree-slp-vectorize
CFLAGSNOOP = -Wall -ffreestanding -nostartfiles

all: clean kernel8.img

$(BUILD)%.o: $(SOURCE)%.s
	$(COMPILER_ROOT)gcc -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

$(BUILD)%.o: $(SOURCE)%.S
	$(COMPILER_ROOT)gcc -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

$(BUILD)%.o: $(SOURCE)%.c
	$(COMPILER_ROOT)gcc -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

$(FONTOBJ): $(FONT)
	$(COMPILER_ROOT)ld -r -b binary -o $(FONTOBJ) $(FONT)

$(AUDIOOBJ): $(AUDIO)
	$(COMPILER_ROOT)ld -r -b binary -o $(AUDIOOBJ) $(AUDIO)
	
$(BUILD)%.o: $(NONOPTIMISED)%.c
	$(COMPILER_ROOT)gcc $(CFLAGSNOOP) -c $< -o $@


kernel8.img: $(ASMOBJS) $(COBJS) $(FONTOBJ) $(COBJSNOOP) $(AUDIOOBJ)
#	$(COMPILER_ROOT)ld -nostartfiles $(FONTOBJ) $(AUDIOOBJ) $(ASMOBJS) $(COBJSNOOP) $(COBJS) -T $(LINKER) -o $(BUILD)kernel.elf
	$(COMPILER_ROOT)gcc -nostartfiles $(FONTOBJ) $(AUDIOOBJ) $(ASMOBJS) $(COBJSNOOP) $(COBJS) -T $(LINKER) -o $(BUILD)kernel.elf -lc -lm -lgcc
	$(COMPILER_ROOT)objcopy $(BUILD)kernel.elf -O binary $(TARGET)

clean:
	$(RM) build$(SLASH)* 
	$(RM) $(TARGET)

run:
	/home/Oliver/Development/qemu/aarch64-softmmu/qemu-system-aarch64 -M raspi3 -kernel kernel8.img -drive file=raspi.iso,if=sd,format=raw -serial stdio
