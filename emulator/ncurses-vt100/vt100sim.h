#ifndef VT100SIM_H
#define VT100SIM_H

#include "nvr.h"
#include "keyboard.h"
#include <stdint.h>
#include <set>

extern "C" {
#include "8080/sim.h"
}

class Vt100Sim
{
public:
  Vt100Sim(char* romPath = 0,bool color=true);
  ~Vt100Sim();
  void init();
  BYTE ioIn(BYTE addr);
  void ioOut(BYTE addr, BYTE data);
  NVR nvr;
  Keyboard kbd;
private:
  char* romPath;
  bool running;
  bool inputMode;
  bool needsUpdate;
  std::set<uint16_t> breakpoints;
  bool dc11, dc12;
public:
  void step();
  void run();
  void keypress(uint8_t keycode);
  void clearBP(uint16_t bp);
  void addBP(uint16_t bp);
  void clearAllBPs();
public:
    void dispRegisters();
    void dispVideo();
    void dispLEDs(uint8_t status);
  void dispMemory();
  void update();
private:
  
};

extern Vt100Sim* sim;

#endif // VT100SIM_H
