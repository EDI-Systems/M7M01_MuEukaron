arm-none-eabi-objcopy -O binary RME RME.bin
arm-none-eabi-objdump -S RME > RME.asm
cd ..
# mkimage -f RME.its RME.itb
# cp -f RME.itb /var/lib/tftpboot/RME.itb
cp -f Debug/RME.bin /var/lib/tftpboot/RME.bin
echo "copy complete"
