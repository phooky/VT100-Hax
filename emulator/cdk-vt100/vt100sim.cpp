
#include "vt100sim.h"
#include "8080/sim.h"
#include "8080/simglb.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>

extern "C" {
extern void int_on(void), int_off(void);
extern void init_io(void), exit_io(void);

extern void cpu_z80(void), cpu_8080(void);
extern void disass(unsigned char **, int);
extern int exatoi(char *);
extern int getkey(void);
extern void int_on(void), int_off(void);
extern int load_file(char *);

}

Vt100Sim* sim;

WINDOW* regWin;
WINDOW* memWin;

Vt100Sim::Vt100Sim(char* romPath, bool color)
{
  initscr();
  int my,mx;
  getmaxyx(stdscr,my,mx);
  if (color) { start_color(); }
  cbreak();
    this->romPath = romPath;
    curs_set(0);
    regWin = subwin(stdscr,8,12,0,0);
    memWin = subwin(stdscr,8,mx-12,0,12);
    box(regWin,0,0);
    box(memWin,0,0);
    init_pair(1,COLOR_RED,COLOR_BLACK);
    init_pair(2,COLOR_BLUE,COLOR_BLACK);
    wattron(regWin,COLOR_PAIR(1));
    wattron(memWin,COLOR_PAIR(2));
    refresh();
}

Vt100Sim::~Vt100Sim() {
  curs_set(1);
  endwin();
}

class Signal {
private:
    bool value;
    bool has_change;
    uint16_t period_half;
    uint16_t ticks;
public:
    Signal(uint16_t period);
    bool add_ticks(uint16_t delta);
    bool get_value() { return value; }
};

Signal::Signal(uint16_t period) : value(false),has_change(false),period_half(period/2),ticks(0) {
}

bool Signal::add_ticks(uint16_t delta) {
    ticks += delta;
    if (ticks >= period_half) {
        ticks -= period_half;
        value = !value;
        return true;
    }
    return false;
}

// In terms of processor cycles:
// LBA4 : period of 22 cycles
// LBA7 : period of 182 cycles
// Vertical interrupt: period of 46084 cycles
Signal lba4(22);
Signal lba7(182);
Signal vertical(46084);

void Vt100Sim::init() {
    i_flag = 1;
    f_flag = 10;
    m_flag = 0;
    tmax = f_flag*10000;
    cpu = I8080;
    //printf("\nRelease %s, %s\n", RELEASE, COPYR);
    //if (f_flag > 0)
    //    printf("\nCPU speed is %d MHz\n", f_flag);
    //else
    //    printf("\nCPU speed is unlimited\n");
#ifdef	USR_COM
    //printf("\n%s Release %s, %s\n", USR_COM, USR_REL, USR_CPR);
#endif
    fflush(stdout);

    //printf("Prep ram\n");
    //fflush(stdout);
    wrk_ram	= PC = ram;
    STACK = ram + 0xffff;
    if (cpu == I8080)	/* the unused flag bits are documented for */
        F = 2;		/* the 8080, so start with bit 1 set */
    memset((char *)	ram, m_flag, 65536);
    // load binary
    //printf("Loading rom %s...\n",romPath);
    fflush(stdout);
    FILE* romFile = fopen(romPath,"rb");
    if (!romFile) {
        printf("Failed to read rom file\n");
        fflush(stdout);
        return;
    }
    uint32_t count = fread((char*)ram,1,2048*4,romFile);
    //printf("Read ROM file; %u bytes\n",count);
    fclose(romFile);
    int_on();
    // add local io hooks

    i_flag = 0;

    // We are always running the CPU in single-step mode so we can do the clock toggles when necessary.
    cpu_state = SINGLE_STEP;
}

BYTE Vt100Sim::ioIn(BYTE addr) {
    if (addr == 0x42) {
        // Read buffer flag
        uint8_t flags = 0x02;
        if (lba7.get_value()) {
            flags |= 0x40;
        }
        if (nvr.data()) {
            flags |= 0x20;
        }
        if (t_ticks % 2000 < 100) { flags |= 0x10; flags |= 0x80; }
        if (kbd.get_tx_buf_empty()) {
            flags |= 0x80; // kbd ready?
        }
        //printf(" IN PORT %02x -- %02x\n",addr,flags);fflush(stdout);
        return flags;
    } else {
      //printf(" IN PORT %02x at %04lx\n",addr,PC-ram);fflush(stdout);
    }
    return 0;
}

void Vt100Sim::ioOut(BYTE addr, BYTE data) {
    switch(addr) {
    case 0x82:
        //printf("OUT PORT %02x <- %02x\n",addr,data);
        //fflush(stdout);
        kbd.set_status(data);
        break;
    case 0x62:
        //printf("NVRAM %02x\n",data);
        //fflush(stdout);
        nvr.set_latch(data);
        break;
    default:
      //printf("OUT PORT %02x <- %02x\n",addr,data);fflush(stdout);
        break;
    }
}

void Vt100Sim::step()
{
  const uint32_t start = t_ticks;
  cpu_error = NONE;
  cpu_8080();
  int_data = 0xff;
  const uint16_t t = t_ticks - start;
  if (lba4.add_ticks(t)) {
    if (kbd.clock(lba4.get_value())) {
      int_data &= 0xcf;
      int_int = 1;
    }
  }
  if (lba7.add_ticks(t)) {
    nvr.clock(lba7.get_value());
  }
  if (vertical.add_ticks(t)) {
    if (vertical.get_value()) {
      int_data &= 0xe7;
    }
  }
  dispRegisters();
  dispMemory();
}

void Vt100Sim::keypress(uint8_t keycode)
{
    kbd.keypress(keycode);
}

void Vt100Sim::dispRegisters() {
  mvwprintw(regWin,1,1,"A %02x",A);
  mvwprintw(regWin,2,1,"B %02x C %02x",B,C);
  mvwprintw(regWin,3,1,"D %02x E %02x",D,E);
  mvwprintw(regWin,4,1,"H %02x L %02x",H,L);
  mvwprintw(regWin,5,1,"PC %04x",(PC-ram));
  mvwprintw(regWin,6,1,"SP %04x",(STACK-ram));
  wrefresh(regWin);
}

void Vt100Sim::dispVideo() {
}

void Vt100Sim::dispLEDs(uint8_t leds) {
}

void Vt100Sim::dispMemory() {
  int my,mx;
  getmaxyx(memWin,my,mx);
  uint16_t start = 0x2000;
  for (int y = 1; y < my - 1; y++) {
    mvwprintw(memWin,y,1,"%04x: ",start);
    start += 32;
  }
  wrefresh(memWin);
}

extern "C" {
BYTE io_in(BYTE addr);
void io_out(BYTE addr, BYTE data);
void exit_io();
}

void exit_io() {}

BYTE io_in(BYTE addr)
{
    return sim->ioIn(addr);
}

void io_out(BYTE addr, BYTE data)
{
    sim->ioOut(addr,data);
}
