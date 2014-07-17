#ifndef VT100SIM_H
#define VT100SIM_H

#include "nvr.h"
#include "keyboard.h"
#include <stdint.h>

extern "C" {
#include "8080/sim.h"
}

class Vt100Sim
{
public:
    Vt100Sim(char* romPath = 0);
    void init();
    BYTE ioIn(BYTE addr);
    void ioOut(BYTE addr, BYTE data);
    NVR nvr;
    Keyboard kbd;
private:
    char* romPath;
public:
    void step();
    void keypress(uint8_t keycode);
};

extern Vt100Sim* sim;

#endif // VT100SIM_H
