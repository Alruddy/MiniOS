#!/bin/bash
dd if=./mbr.bin of=/opt/bochs/hd60M.img bs=512 count=1 conv=notrunc
