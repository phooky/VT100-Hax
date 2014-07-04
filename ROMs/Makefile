TARGET=haxrom.bin
AS=/usr/local/bin/asm8080
ORIGINAL_ASM=basic.d80

*.bin: *.d80
	$(AS) -l$<.lst $<

testasm:
	cp basic.d80 test.d80
	$(AS) test.d80
	truncate --size=8K test.bin
	cmp -l test.bin basic.bin

all: haxrom.bin
