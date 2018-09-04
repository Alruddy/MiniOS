#!/bin/bash

gcc -I lib/kernel -c -o build/timer.o device/timer.c 
gcc -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/main.o kernel/main.c
gcc -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/init.o kernel/init.c
gcc -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/interrupt.o kernel/interrupt.c

nasm -f elf -o build/kernel.o kernel/kernel.S 
nasm -f elf -o build/print.o lib/kernel/print.S

ld -Ttext 0xc0001500 -e main -o build/kernel.bin \
    build/main.o build/timer.o build/init.o build/interrupt.o \
    build/kernel.o build/print.o

dd if=build/kernel.bin of=/opt/bochs/hd60M.img \
bs=512 count=200 seek=9 conv=notrunc