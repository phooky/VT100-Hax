Notes on the VT100 Platform
===========================

Questions:
----------
* Do we have the advanced video option (AVO)?

page 60 of the manual: 8080 overview diagram
page 62: overview of vt100 board
page 94: nonvolatile RAM 

Jul82 Technical Manual Errata:
------------------------------
The diagram on page 5-69 has the order of the ROMs reversed.

Overview
--------

* Processor: Intel 8080
* Basic display modes: 80x24 or 132x14 (132x24 for avo)
* RAM: 3KB(3072 bytes) combined screen and scratch
* ROM: 8KB(8192 bytes) for processor data and code
** ROMs include a checksum which is checked at self-test
** Checksum: every 2K, summed, mod 0xff == 0
* Character generation ROM(s): 2KB(2048 bytes) for characters
* Non-volatile RAM for storing configuration

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

Relevant port addresses
-----------------------

Location  | R/W | Description
----------|-----|------------
0x00      | R/W | PUSART data bus
0x01      | R/W | PUSART command port
0x42      |  W  | Brightness D/A latch
0x42      |  R  | Flags buffer
0x62      |  W  | Non-volatile memory latch
0x82      |  W  | Keyboard write
0x82      |  R  | Keyboad read
0xA2      |  W  | vid. proc. DC012
0xC2      |  W  | vid. proc. DC011

Flags buffer
------------

Bit  | Active? | Description
-----|---------|------------
7    | H       | KBD TBMT(?)
6    | H       | LBA 7(?) (It's a pin on the backplane connector...)
5    | H       | NVR DATA
4    | L       | EVEN FIELD (comes out of the video timing generator)
3    | H       | OPTION PRESENT
2    | L       | GRAPHICS FLAG
1    | L       | ADVANCED VIDEO (is AVO present?) 
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



Control Function Parser
-----------------------

Scans for characters in the FIFO in the control range (<0x20 or 0x7f).

8-bit data bus, 16-bit address bus.
8228 breaks devices on bus into write-only and bidirectional/read-only busses.
