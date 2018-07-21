#!/bin/bash
dd if=./loader.bin of=/opt/bochs/hd60M.img bs=512 count=1 seek=2 conv=notrunc
