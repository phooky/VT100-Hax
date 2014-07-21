#include "keyboard.h"
#include <stdio.h>
#include "8080/sim.h"
#include "8080/simglb.h"
#include <ncurses.h>

Keyboard::Keyboard() : state(KBD_IDLE), latch(0), tx_buf_empty(true)
{
    scan_iter = scan.end();
}

uint8_t Keyboard::get_latch()
{
    return latch;
}

bool Keyboard::get_tx_buf_empty() { return tx_buf_empty; }

extern WINDOW* msgWin;

void Keyboard::set_status(uint8_t status)
{
  last_status = status;
    //printf("Got kbd status %02x at %04x\n",status,PC-ram); fflush(stdout);
    if ((status & (1<<6)) &&  state == KBD_IDLE) {
        //printf("SCAN START\n");fflush(stdout);
      //wprintw(msgWin,"Scan start\n");wrefresh(msgWin);
        state = KBD_SENDING;
        clocks_until_next = 160;

    }
}

void Keyboard::keypress(uint8_t keycode)
{
  //printf("PRESS %02x\n",keycode);fflush(stdout);
    keys.insert(keycode);
}

bool Keyboard::clock(bool rising)
{
    if (!rising) { return false; }
    switch (state) {
    case KBD_IDLE:
        break;
    case KBD_SENDING:
        if (clocks_until_next == 0) {
            scan = keys;
            keys.clear();
            scan_iter = scan.begin();
            state = KBD_RESPONDING;
            clocks_until_next = 160;
            if (scan_iter == scan.end()) { clocks_until_next += 127; } else { clocks_until_next += *scan_iter; }
            //printf("RSP MODE\n");fflush(stdout);
        } else {
            clocks_until_next--;
        }
        break;
    case KBD_RESPONDING:
        if (clocks_until_next == 0) {
            if (scan_iter != scan.end()) {
	      //wprintw(msgWin,"Sending %02x\n",*scan_iter);wrefresh(msgWin);
	      //printf("SENDING KEY %02x\n",*scan_iter);fflush(stdout);
                clocks_until_next = 160;
                latch = *scan_iter;
		if (latch != last_sent) {
		  // hack around debounce problem
		  keys.insert(latch);
		  last_sent = latch;
		}
                scan_iter++;
            } else {
                latch = 0x7f;
                state = KBD_IDLE;
		//wprintw(msgWin,"End scan\n");wrefresh(msgWin);
            }
            return true;
        }
        clocks_until_next--;
        return false;
        break;
    default:
      wprintw(msgWin,"Bad state\n");wrefresh(msgWin);
      
    }
    return false;
}


