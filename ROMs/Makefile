TARGET=haxrom.bin
AS=/usr/local/bin/asm8080
ORIGINAL_ASM=basic.d80

all: haxrom.bin

%.bin: %.d80
	$(AS) -l$*.lst -o$* $<
	truncate --size=8K $*.bin

test.d80: basic.d80
	cp basic.d80 test.d80

testasm: test.bin
	cmp -l test.bin basic.bin
