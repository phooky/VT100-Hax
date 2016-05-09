#ifndef VT100SIM_H
#define VT100SIM_H

#include "nvr.h"
#include "keyboard.h"
#include "pusart.h"
#include <stdint.h>
#include <set>
#include <sys/time.h>

extern "C" {
#include "8080/sim.h"
}

class Vt100Sim
{
public:
  Vt100Sim(const char* romPath = 0,bool running=false, bool avo_on=true);
  ~Vt100Sim();
  void init();
  BYTE ioIn(BYTE addr);
  void ioOut(BYTE addr, BYTE data);
  NVR nvr;
  Keyboard kbd;
  PUSART uart;
  uint8_t bright;
private:
  const char* romPath;
  bool running;
  bool inputMode;
  bool needsUpdate;
  std::set<uint16_t> breakpoints;
  bool has_breakpoints;
  bool dc12;
  bool controlMode;
  bool enable_avo;
  long long rt_ticks;
  struct timeval last_sync;
  int scroll_latch;
  int screen_rev;
  int base_attr;
  int blink_ff;
public:
  void getString(const char* prompt, char* buffer, uint8_t sz);
  void step();
  void run();
  void keypress(uint8_t keycode);
  void clearBP(uint16_t bp);
  void addBP(uint16_t bp);
  void clearAllBPs();
public:
    void dispRegisters();
    void dispVideo();
    void dispLEDs();
    void dispStatus();
    void dispBPs();
  void dispMemory();
    void snapMemory();
  void update();
private:
  
};

extern Vt100Sim* sim;

#endif // VT100SIM_H
