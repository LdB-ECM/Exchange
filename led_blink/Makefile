ARMGNU = arm-none-eabi
AFLAGS = -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv8-a

CFLAGS = -Wall -O2 -nostartfiles -nostdlib -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv8-a -mtune=cortex-a53 -g -c

INC = -I ./

all : kernel.img

startup.o : startup.s
	$(ARMGNU)-as $(AFLAGS) startup.s -o startup.o

main.o : main.c
	$(ARMGNU)-gcc $(CFLAGS) $(INC) main.c -o main.o


output.elf: main.o startup.o
	$(ARMGNU)-ld  startup.o main.o -T linker.ld -Map=output.map -o  output.elf

kernel.img: output.elf
	$(ARMGNU)-objcopy output.elf -O binary kernel8-32.img
	$(ARMGNU)-objdump -D output.elf > output.list
	$(ARMGNU)-nm -n output.elf > output.symbol
	rm -f *.o

clean:
	rm -f *.o *.elf *.img *.map *.list *.symbol

