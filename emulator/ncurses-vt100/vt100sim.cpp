
#include "vt100sim.h"
#include "8080/sim.h"
#include "8080/simglb.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <signal.h>
#include <map>
#include <ctype.h>

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
WINDOW* vidWin;
WINDOW* msgWin;
WINDOW* statusBar;
WINDOW* bpWin;

Vt100Sim::Vt100Sim(const char* romPath, bool running) : running(running), inputMode(false),
							dc11(true), dc12(false), controlMode(true)
{
  this->romPath = romPath;

  //breakpoints.insert(8);
  //breakpoints.insert(0xb);
  initscr();
  int my,mx;
  getmaxyx(stdscr,my,mx);
  start_color();
  cbreak();
  noecho();
  keypad(stdscr,1);
  nodelay(stdscr,1);
  curs_set(0);

  // Status bar: bottom line of the screen
  statusBar = subwin(stdscr,1,mx,--my,0);
  const int vht = std::min(27,my-12); // video area height (max 27 rows)
  const int memw = 7 + 32*3 + 2; // memory area width: big enough for 32B across
  const int regw = 12;
  const int regh = 8;
  const int msgw = mx - (regw+memw); // message area: mx - memory area - register area (12)

  vidWin = subwin(stdscr,vht,mx,my-vht,0);
  regWin = subwin(stdscr,regh,regw,0,0);
  bpWin = subwin(stdscr,my-(vht+regh),regw,regh,0);
  memWin = subwin(stdscr,my-vht,memw,0,regw);
  msgWin = subwin(stdscr,my-vht,msgw,0,regw+memw);

  scrollok(msgWin,1);
  box(regWin,0,0);
  mvwprintw(regWin,0,1,"Registers");
  box(memWin,0,0);
  mvwprintw(memWin,0,1,"Mem");
  box(vidWin,0,0);
  mvwprintw(vidWin,0,1,"Video");
  box(bpWin,0,0);
  mvwprintw(bpWin,0,1,"Brkpts");
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
    wprintw(msgWin,"\nRelease %s, %s\n", RELEASE, COPYR);
    if (f_flag > 0)
      wprintw(msgWin,"\nCPU speed is %d MHz\n", f_flag);
    else
      wprintw(msgWin,"\nCPU speed is unlimited\n");
#ifdef	USR_COM
    wprintw(msgWin,"\n%s Release %s, %s\n", USR_COM, USR_REL, USR_CPR);
#endif

    //printf("Prep ram\n");
    //fflush(stdout);
    wrk_ram	= PC = ram;
    STACK = ram + 0xffff;
    if (cpu == I8080)	/* the unused flag bits are documented for */
        F = 2;		/* the 8080, so start with bit 1 set */
    memset((char *)	ram, m_flag, 65536);
    memset((char *)	touched, m_flag, 65536);
    // load binary
    wprintw(msgWin,"Loading rom %s...\n",romPath);
    wrefresh(msgWin);
    FILE* romFile = fopen(romPath,"rb");
    if (!romFile) {
      wprintw(msgWin,"Failed to read rom file\n");
      wrefresh(msgWin);
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
    } else if (addr == 0x82) {
      return kbd.get_latch();
    } else {
      //printf(" IN PORT %02x at %04lx\n",addr,PC-ram);fflush(stdout);
    }
    return 0;
}

void Vt100Sim::ioOut(BYTE addr, BYTE data) {
    switch(addr) {
    case 0x82:
        kbd.set_status(data);
        break;
    case 0x62:
        nvr.set_latch(data);
	break;
    case 0x42:
      bright = data;
      break;
    case 0xa2:
      //wprintw(msgWin,"DC11 %02x\n",data);
      //wrefresh(msgWin);
      dc11 = true;
      break;
    case 0xc2:
      //wprintw(msgWin,"DC12 %02x\n",data);
      //wrefresh(msgWin);
      dc12 = true;
        break;

    default:
      //printf("OUT PORT %02x <- %02x\n",addr,data);fflush(stdout);
        break;
    }
}

volatile sig_atomic_t sigAlrm = 0;

std::map<int,uint8_t> make_code_map() {
  std::map<int,uint8_t> m;
  m[KEY_DC] = 0x03;
  m[KEY_ENTER] = 0x04;
  m['\r'] = 0x04;
  m['p'] = 0x05;
  m['o'] = 0x06;
  m['y'] = 0x07;
  m['t'] = 0x08;
  m['w'] = 0x09;
  m['q'] = 0x0a;

  m[KEY_RIGHT] = 0x10;
  m[']'] = 0x14; m['}'] = 0x94;
  m['['] = 0x15; m['{'] = 0x95;
  m['i'] = 0x16;
  m['u'] = 0x17;
  m['r'] = 0x18;
  m['e'] = 0x19;
  m['1'] = 0x1a; m['!'] = 0x9a;

  m[KEY_LEFT] = 0x20;
  m[KEY_DOWN] = 0x22;
  m[KEY_BREAK] = 0x23;
  m['`'] = 0x24; m['~'] = 0xa4;
  m['-'] = 0x25; m['_'] = 0xa5;
  m['9'] = 0x26; m['('] = 0xa6;
  m['7'] = 0x27; m['&'] = 0xa7;
  m['4'] = 0x28; m['$'] = 0xa8;
  m['3'] = 0x29; m['#'] = 0xa9;
  m[KEY_CANCEL] = 0x2a;

  m[KEY_UP] = 0x30;
  m[KEY_F(3)] = 0x31;
  m[KEY_F(1)] = 0x32;
  m[KEY_BACKSPACE] = 0x33;
  m['='] = 0x34; m['+'] = 0xb4;
  m['0'] = 0x35; m[')'] = 0xb5;
  m['8'] = 0x36; m['*'] = 0xb6;
  m['6'] = 0x37; m['^'] = 0xb7;
  m['5'] = 0x38; m['%'] = 0xb8;
  m['2'] = 0x39; m['@'] = 0xb9;
  m['\t'] = 0x3a;

  m[KEY_F(4)] = 0x41;
  m[KEY_F(2)] = 0x42;
  m['\n'] = 0x44;
  m['\\'] = 0x45; m['|'] = 0xc5;
  m['l'] = 0x46;
  m['k'] = 0x47;
  m['g'] = 0x48;
  m['f'] = 0x49;
  m['a'] = 0x4a;

  m['\''] = 0x55; m['"'] = 0xd5;
  m[';'] = 0x56; m[':'] = 0xd6;
  m['j'] = 0x57;
  m['h'] = 0x58;
  m['d'] = 0x59;
  m['s'] = 0x5a;

  m['.'] = 0x65; m['>'] = 0xe5;
  m[','] = 0x66; m['<'] = 0xe6;
  m['n'] = 0x67;
  m['b'] = 0x68;
  m['x'] = 0x69;

  m['/'] = 0x75; m['?'] = 0xf5;
  m['m'] = 0x76;
  m[' '] = 0x77;
  m['v'] = 0x78;
  m['c'] = 0x79;
  m['z'] = 0x7a;
  
  // setup
  m['`'] = 0x7b; m['~'] = 0x7b;

  for (int i = 0; i < 26; i++) {
    m['A'+i] = m['a'+i] | 0x80;
  }
  return m;
}

std::map<int,uint8_t> code = make_code_map();

void sig_alrm(int signo) { sigAlrm = 1; }

bool hexParse(char* buf, int n, uint16_t& d) {
  d = 0;
  for (int i = 0; i < n; i++) {
    char c = buf[i];
    if (c == 0) return true;
    d = d << 4;
    if (c >= '0' && c <= '9') { d |= (c-'0'); }
    else if (c >= 'a' && c <= 'f') { d |= ((c-'a')+10); }
    else if (c >= 'A' && c <= 'F') { d |= ((c-'A')+10); }
    else {
      return false;
    }
  }
  return true;
}

void Vt100Sim::run() {
  signal(SIGALRM,sig_alrm);
  ualarm(5000,5000);
  int steps = 0;
  needsUpdate = true;
  while(1) {
    if (running) {
      step();
      if (steps > 0) {
	if (--steps == 0) { running = false; }
      }
      uint16_t pc = (uint16_t)(PC-ram);
      //wprintw(msgWin,"BP %d PC %d\n",breakpoints.size(),pc);wrefresh(msgWin);
      if (breakpoints.find(pc) != breakpoints.end()) {
	running = false;
      }
    } else {
      usleep(5000);
    }
    if (sigAlrm && needsUpdate) { sigAlrm = 0; update();}
    int ch = getch();
    if (ch != ERR) {
      if (ch == KEY_F(2)) {
	controlMode = !controlMode;
	dispStatus();
      } else if (controlMode) {
	if (ch == 'q') {
	  return;
	}
	else if (ch == ' ') {
	  running = !running;
	}
	else if (ch == 'n') {
	  running = true; steps = 1;
	}
	else if (ch == 'm') {
	  snapMemory(); dispMemory();
	}
	else if (ch == 'b') {
	  char bpbuf[10];
	  getString("Addr. of breakpoint: ",bpbuf,4);
	  werase(statusBar);
	  dispStatus();
	  uint16_t bp;
	  if (hexParse(bpbuf,4,bp)) {
	    addBP(bp);
	    dispBPs();
	    mvwprintw(statusBar,0,0,"Breakpoint addded at %s\n",bpbuf); 
	  } else {
	    mvwprintw(statusBar,0,0,"Bad breakpoint %s\n",bpbuf); 
	  }
	  // set up breakpoints
	}
	else if (ch == 'd') {
	  char bpbuf[10];
	  getString("Addr. of bp to remove: ",bpbuf,4);
	  werase(statusBar);
	  dispStatus();
	  uint16_t bp;
	  if (hexParse(bpbuf,4,bp)) {
	    if (breakpoints.count(bp) == 0) {
	      mvwprintw(statusBar,0,0,"No breakpoint %s\n",bpbuf); 
	    } else {
	      clearBP(bp);
	      dispBPs();
	      mvwprintw(statusBar,0,0,"Breakpoint removed at %s\n",bpbuf); 
	    }
	  } else {
	    mvwprintw(statusBar,0,0,"Bad breakpoint %s\n",bpbuf); 
	  }
	  // set up breakpoints
	}
      }
      else {
	uint8_t kc = code[ch];
	if (kc & 0x80) {
	  keypress(0x7d);
	  kc &= 0x7f;
	}
	keypress(kc);
      }
    }
  }
}

void Vt100Sim::getString(const char* prompt, char* buf, uint8_t sz) {
  uint8_t l = strlen(prompt);
  mvwprintw(statusBar,0,0,prompt);
  echo();
  curs_set(1);
  wgetnstr(statusBar,buf,sz);
  noecho();
  curs_set(0);
  werase(statusBar);
  dispStatus();
}

void Vt100Sim::step()
{
  const uint32_t start = t_ticks;
  cpu_error = NONE;
  cpu_8080();
  if (int_int == 0) { int_data = 0xff; }
  const uint16_t t = t_ticks - start;
  if (dc11 && lba4.add_ticks(t)) {
    if (kbd.clock(lba4.get_value())) {
      int_data &= 0xcf;
      int_int = 1;
      //wprintw(msgWin,"KBD interrupt\n");wrefresh(msgWin);
    }
  }
  if (dc11 && lba7.add_ticks(t)) {
    nvr.clock(lba7.get_value());
  }
  if (dc11 && vertical.add_ticks(t)) {
    if (vertical.get_value()) {
      int_data &= 0xe7;
      int_int = 1;
    }
  }
  needsUpdate = true;
}

void Vt100Sim::update() {
  needsUpdate = false;
  dispRegisters();
  dispMemory();
  dispVideo();
  dispStatus();
  dispBPs();
}

void Vt100Sim::keypress(uint8_t keycode)
{
    kbd.keypress(keycode);
}

void Vt100Sim::clearBP(uint16_t bp)
{
  breakpoints.erase(bp);
}

void Vt100Sim::addBP(uint16_t bp)
{
  breakpoints.insert(bp);
}

void Vt100Sim::clearAllBPs()
{
  breakpoints.clear();
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
  uint16_t start = 0x2000;
  werase(vidWin);
  box(vidWin,0,0);
  mvwprintw(vidWin,0,1,"Video [bright %x]",bright);
  uint8_t y = -2;
  for (uint8_t i = 1; i < 100; i++) {
        char* p = (char*)ram + start;
        char* maxp = p + 133;
	//if (*p != 0x7f) y++;
	y++;
	wmove(vidWin,y,1);
        while (*p != 0x7f && p != maxp) {
            unsigned char c = *(p++);
	    if (y > 0) {
	      if (c == 0 || c == 127) {
		waddch(vidWin,' ');
	      } else  {
		bool inverse = c > 127;
		if (inverse) {
		  c-=128;
		  wattron(vidWin,A_REVERSE);
		}
		if (c < 7) { waddch(vidWin,' '); } 
		else { waddch(vidWin,c); }
		if (inverse) wattroff(vidWin,A_REVERSE);
		//wprintw(vidWin,"%02x",c);
	      }
            }
        }
        if (p == maxp) {
	  wprintw(msgWin,"Overflow line %d\n",i); wrefresh(msgWin);
	  break;
	}
        // at terminator
        p++;
        unsigned char a1 = *(p++);
        unsigned char a2 = *(p++);
        //printf("Next: %02x %02x\n",a1,a2);fflush(stdout);
        uint16_t next = (((a1&0x10)!=0)?0x2000:0x4000) | ((a1&0x0f)<<8) | a2;
        if (start == next) break;
        start = next;
    }
  wrefresh(vidWin);
}

void displayFlag(const char* text, bool on) {
  if (on) {
    wattron(statusBar,COLOR_PAIR(2));
    wattron(statusBar,A_BOLD);
  } else {
    wattron(statusBar,COLOR_PAIR(1));
    wattroff(statusBar,A_BOLD);
  }
  wprintw(statusBar,text);
  waddch(statusBar,' ');
  waddch(statusBar,' ');
}

void Vt100Sim::dispStatus() {
  // ONLINE LOCAL KBD LOCK L1 L2 L3 L4
  const static char* ledNames[] = { 
    "ONLINE", "LOCAL", "KBD LOCK", "L1", "L2", "L3", "L4" };
  const int lwidth = 8 + 7 + 10 + 4 + 4 + 4 + 4;
  int mx, my;
  getmaxyx(statusBar,my,mx);
  wmove(statusBar,0,mx-lwidth);
  uint8_t flags = kbd.get_status();
  displayFlag(ledNames[0], (flags & (1<<5)) != 0 );
  displayFlag(ledNames[1], (flags & (1<<5)) == 0 );
  displayFlag(ledNames[2], (flags & (1<<4)) != 0 );
  displayFlag(ledNames[3], (flags & (1<<3)) != 0 );
  displayFlag(ledNames[4], (flags & (1<<2)) != 0 );
  displayFlag(ledNames[5], (flags & (1<<1)) != 0 );
  displayFlag(ledNames[6], (flags & (1<<0)) != 0 );

  // Mode information
  wmove(statusBar,0,mx/2);
  wattrset(statusBar,A_BOLD);
  wprintw(statusBar,"| ");
  if (controlMode) {
    wprintw(statusBar,"CONTROL");
  } else {
    wattron(statusBar,A_REVERSE);
    wprintw(statusBar,"TYPING");
    wattroff(statusBar,A_REVERSE);
    wprintw(statusBar," ");
  }
  wprintw(statusBar," | ");
  if (running) {
    wprintw(statusBar,"RUNNING");
  } else {
    wattron(statusBar,A_REVERSE);
    wprintw(statusBar,"STOPPED");
    wattroff(statusBar,A_REVERSE);
  }
  wprintw(statusBar," |");
  wrefresh(statusBar);
}

void Vt100Sim::dispBPs() {
  int y = 1;
  werase(bpWin);
  box(bpWin,0,0);
  mvwprintw(bpWin,0,1,"Brkpts");
  for (std::set<uint16_t>::iterator i = breakpoints.begin();
       i != breakpoints.end();
       i++) {
    mvwprintw(bpWin,y++,2,"%04x",*i);
  }
  wrefresh(bpWin);
}

void Vt100Sim::snapMemory() {
  memset(touched+0x2000,0,0x2000);
}

void Vt100Sim::dispMemory() {
  int my,mx;
  getmaxyx(memWin,my,mx);
  int bavail = (mx - 7)/3;
  int bdisp = 8;
  while (bdisp*2 <= bavail) bdisp*=2;
  uint16_t start = 0x2000;
  
  wattrset(memWin,A_NORMAL);
  for (int b = 0; b<bdisp;b++) {
    mvwprintw(memWin,0,7+3*b,"%02x",b);
  }
  wattrset(memWin,COLOR_PAIR(1));

  for (int y = 1; y < my - 1; y++) {
    wattrset(memWin,COLOR_PAIR(1));
    mvwprintw(memWin,y,1,"%04x:",start);
    for (int b = 0; b<bdisp;b++) {
      if (!touched[start]) 
	wattron(memWin,A_STANDOUT);
      if (ram[start] != 00) {
	wattron(memWin,COLOR_PAIR(2));
	wprintw(memWin," %02x",ram[start++]);
	wattron(memWin,COLOR_PAIR(1));
      } else {
	wprintw(memWin," %02x",ram[start++]);
      }
      wattroff(memWin,A_STANDOUT);
    }
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
