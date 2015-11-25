@echo off
avrdude -c USBasp -p m328p -U lfuse:w:0xFF:m -U hfuse:w:0xD9:m -U efuse:w:0x06:m
echo Fuses Written at:
time /T
pause

