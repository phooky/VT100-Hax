Notes on the VT100 Platform
===========================

Overview
--------

* Processor: Intel 8080 running at about 2.76MHz
* Basic display modes: 80x24 or 132x14 (132x24 for avo)
* RAM: 3KB(3072 bytes) combined screen and scratch
* ROM: 8KB(8192 bytes) for processor data and code
** ROMs include a checksum which is checked at self-test
** Checksum: every 2K, summed, mod 0xff == 0
* Character generation ROM(s): 2KB(2048 bytes) for characters
* Non-volatile RAM for storing configuration, ER1400 (1400 bits)

* AVO Option: 1K extra main RAM, 4K of FOUR BIT RAM for attributes.

Clocks
------

Main Crystal: 24.8832 MHz
NVR/LBA7: 15.734 kHz (63.556 uS); estimate at 8 * LBA4
LBA4 period : 7.945uS
Even line: 
Kbd transmission time : 1.27mS/B
Vertical freq: ~60 Hz
Dot clock: 24 MHz in 132-char mode; 16 MHz in 80-char mode
PSUART: 2.76480 MHz (Div9)
Processor: 2.76480 MHz (Div9)

Memory Map
----------

 Start  |   End   |  Size  |  Description
--------|---------|--------|-------------
0x0000  | 0x1fff  |   8K   | Basic ROM
0x2000  | 0x2bff  |   3K   | Video and scratch RAM (see below)
0x2c00  | 0x2fff  |   1K   | AVO - screen RAM
0x3000  | 0x3fff  |   4K   | AVO - attribute RAM
0x4000  | 0x7fff  |  16K   | Unassigned
0x8000  | 0x9fff  | 2Kx4   | AVO - program memory expansion ROM
0xa000  | 0xbfff  |   8K   | AVO - program memory expansion ROM
0xc000  | 0xffff  |  16K   | Unassigned

ROM is four 2kx8 mask programmed ROMs.
Main RAM is six 1kx4 static RAM chips.
AVO RAM is another six 1kx4 static RAM chips.

Modifications
-------------

Cleared out from 0x66 to 0xb9 -- 83 bytes
At end of roms -- 1fd7 to 2000 -- 41 bytes

0x2004/0x2005 get saved and set to 70 03 when screensaver is on

 Start  |   End   |  Size  |  Description
--------|---------|--------|-------------
0x22c2  | 0x22c4  |   2B   | SSvfl Vertical frames left until screensaver
0x22c4  | 0x22c5  |   1B   | SSbfl Vertical frames left until next brightness step down
0x22c5  | 0x22c6  |   1B   | SSst  Screensaver state: 00 off, 01 fading, 02 on
0x22c6  | 0x22c8  |   2B   | SSprv Remember previous first line ptr
0x22c8  | 0x22c9  |   1B   | SSbrt Backup of brightness

RAM Map
-------

 Start  |   End   |  Size  |  Description
--------|---------|--------|-------------
0x2000  | 0x2012  |  18B   | Blank lines for refresh (6 x 3B)
0x2012  | 0x204e  |  60B   | Stack area (grows down from 0x204e)
0x204f  | 0x22d0  | 641B   | Scratch Pad/Setup Area(?)
 0x2052  | 0x2054  |   2B   | 0x2004 during init ?
 0x2068  | 0x2069  |   1B   | Keys flag buffer
 0x206a  | 0x206e  |   3B   | New keys pressed buffer
 0x20f6  | 0x20f8  |   2B   | 0x22d0 during init?
 0x2014  | 0x2015  |   1B   | 0xff during init?
0x22d0  | 0x2c00  | 2352B  | Screen RAM

0x22bb through 0x22d0 appear to be unused!!!

We are using ??? as our screensaver counter.

Setup Area

Start   |  Size  |  Description
--------|--------|-------------
0x217b  | 22     | Answerback message (20chars+2delim)
0x2191  | 17     | Tabs (bit encoding) (first bit always set)
0x21a2  | 1      | 80/132 col mode (00 = 80 col, 01 = 132 col)
0x21a3  | 1      | intensity (00 = brightest, 0x1f = dimmest)
0x21a4  | 1      | Mode byte for PUSART
0x21a5  | 1      | Online/local
0x21a6  | 1      | Switches 1
0x21a7  | 1      | Switches 2
0x21a8  | 1      | Switches 3
0x21a9  | 1      | Switches 4
0x21aa  | 1      | Switches 5
0x21ab  | 1      | TX baud rate
0x21ac  | 1      | RX baud rate
0x21ad  | 1      | parity 
0x21ae  | 1      | nvr checksum



Screen RAM organization
-----------------------
Starts at 0x2000. Each line consists of a terminator (0x7f) followed by
two bytes of address and attributes:

7 | 6 5 | 4 | 3 2 1 0 | 7 6 5 4 3 2 1
--|-----|---|---------|--------------
S | AA  | M | addr    | addr

S == 1 if part of scrolling region
AA == 11 if normal atrributes
M == 1 if ram starting at 0x2000, M = 0 if ram starting at 0x4000 (?)
addr == low bits of address

Relevant port addresses
-----------------------

Location  | R/W | Description
----------|-----|------------
0x00      | R/W | PUSART data bus
0x01      | R/W | PUSART command port
0x02      |  W  | Baud rate generator
0x42      |  W  | Brightness D/A latch
0x42      |  R  | Flags buffer
0x62      |  W  | Non-volatile memory latch
0x82      |  W  | Keyboard write
0x82      |  R  | Keyboad read
0xA2      |  W  | vid. proc. DC012
0xC2      |  W  | vid. proc. DC011

There is no even field mystery. It was the LBA 7 (also known as the NVR clock) all along!

NVRAM interactions
------------------
data is D0
C1 is D1
C2 is D2
C3 is D3
!SPDS is D5

Accept address high, accept address low, standby, read, standby, (accept data is standby?)
22 23x9 22 23x9 0x2f 0x2d 0x2f 0x25 0x2f 0x30 0x23

Flags buffer
------------

Bit  | Active? | Description
-----|---------|------------
7    | H       | KBD Transmit Buffer Empty
6    | H       | LBA 7(?) (It's a pin on the backplane connector...) - used to clock NVR - line buffer address
5    | H       | NVR DATA
4    | L       | EVEN FIELD (comes out of the video timing generator)
3    | H       | OPTION PRESENT (terminal output option???)
2    | L       | GRAPHICS FLAG (is VT52 graphics card present)
1    | L       | ADVANCED VIDEO (is AVO present) 
0    | H       | XMIT FLAG

Interrupt vector
----------------

Location  |  Purpose
----------|--------------
0x00      | Startup
0x08      | Keyboard interrupt
0x10      | Receiver
0x18      | Keyboard interrupt and receiver
0x20      | Vertical frequency
0x28      | Vertical frequency and keyboard interrupt
0x30      | Vertical frequency and receiver
0x38      | Vertical frequency and keyboard interrupt and receiver

Other addresses are bitwise combinations of these interrupts! Crazy.

Convenience Functions in ROM
----------------------------

Label  |   Description
-------|----------------------
X13de  |  add A to HL

X1083  |  set DE bytes starting at HL to B
X02a4  |  init: zero all RAM above stack
X038b  |  memmov: copy B bytes from DE to HL
X1083  |  memset: set DE bytes starting at HL to B

Control Function Parser
-----------------------

Scans for characters in the FIFO in the control range (<0x20 or 0x7f).

8-bit data bus, 16-bit address bus.
8228 breaks devices on bus into write-only and bidirectional/read-only busses.

LXI LIST
--------

All value loads that could be addresses:
X01c3: lxi h,X0815 ???
       lxi h, 020e ??? right after loads keymapping table
X0431: lxi b,X0113 (then call 0f7e,0e47)
X05af: lxi h,X05b8 (then jmp a18) // Likely table? (but subsequent code at 5b8 is valid)
X0c64: lxi h,X05ad (then jmp a18) // ... less likely table, feels like a chain
X0c7a: lxi h,X0c8a (feels like a table)

STP -- Standard Terminal Port
-----------------------------

This is an edge connector on the main board that contains breakout points for
the RS232 and Modem lines. It can be used to disconnect the terminal from the
normal serial port and connect it to an internal device. The external serial
port is also available to the device as a plain connector.

