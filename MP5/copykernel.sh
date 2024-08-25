mkdir ./floppy
hdiutil attach dev_kernel_grub.img -mountpoint ./floppy
cp kernel.bin ./floppy
sleep 1
hdiutil detach ./floppy
rmdir ./floppy
