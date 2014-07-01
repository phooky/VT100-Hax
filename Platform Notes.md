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
0x2012  | 0x204f  |  61B   | Stack area (grows down from 0x204e)
0x204f  | 0x22d0  | 641B   | Scratch Pad/Setup Area(?)
0x22d0  | 0x2c00  | 2352B  | Screen RAM

Relevant port addresses
-----------------------

Location  | R/W | Description
----------|-----|------------
0x42      |  W  | Brightness D/A latch
0x82      |  W  | Keyboard write
0x82      |  R  | Keyboad read

Interrupt vector
----------------

Location  |  Purpose
----------|--------------
0x00      | Startup
0x08      | Keyboard interrupt
0x10      | Receiver
0x20      | Vertical frequency

Other addresses are bitwise combinations of these interrupts! Crazy.

Control Function Parser
-----------------------

Scans for characters in the FIFO in the control range (<0x20 or 0x7f).

8-bit data bus, 16-bit address bus.
8228 breaks devices on bus into write-only and bidirectional/read-only busses.
