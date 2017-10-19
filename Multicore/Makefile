ARMGNU = arm-none-eabi

AFLAGS = -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv8-a
CFLAGS = -Wall -O2 -nostartfiles -nostdlib -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv8-a -mtune=cortex-a53 -g -c
LDFLAGS = -lc -lm -lgcc

INC = -I ./

all : kernel.img

SmartStart.o : SmartStart32.S
	$(ARMGNU)-as $(AFLAGS) SmartStart32.S -o SmartStart.o

rpi-SmartStart.o : rpi-SmartStart.c
	$(ARMGNU)-gcc $(CFLAGS) $(INC) $(LDFLAGS) rpi-SmartStart.c -o rpi-SmartStart.o

emb-stdio.o : emb-stdio.c
	$(ARMGNU)-gcc $(CFLAGS) $(INC) $(LDFLAGS) emb-stdio.c -o emb-stdio.o

main.o : main.c
	$(ARMGNU)-gcc $(CFLAGS) $(INC) $(LDFLAGS) main.c -o main.o

output.elf: main.o SmartStart.o rpi-SmartStart.o emb-stdio.o
	$(ARMGNU)-ld  SmartStart.o rpi-SMartStart.o emb-stdio.o main.o -T rpi32.ld -Map=output.map -o  output.elf

kernel.img: output.elf
	$(ARMGNU)-objcopy output.elf -O binary kernel8-32.img
	$(ARMGNU)-objdump -D output.elf > output.list
	$(ARMGNU)-nm -n output.elf > output.symbol
	rm -f *.o

clean:
	rm -f *.o *.elf *.img *.map *.list *.symbol

