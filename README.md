VT100 Hax for Retrocomp 2014
============================

I'm reverse engineering the VT100 firmware and adding some patches
for the 2014 retrocomp. It's crazy and fun!

Quick guide:
------------

* 23-0???.bin - raw dumps of the ROMs on the basic video board.
  These are four 2Kx8 ROMs; a single 8Kx8 version exists as well.
* basic.bin - combined ROM generated by concatenating the 2Kx2 ROMs.
* basic.d80 - disassembled basic ROM `dz80 -80 basic.bin`
