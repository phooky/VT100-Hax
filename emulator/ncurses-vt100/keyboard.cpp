#include "keyboard.h"
#include <stdio.h>
#include "8080/sim.h"
#include "8080/simglb.h"
#include <ncurses.h>

Keyboard::Keyboard() : state(KBD_IDLE), latch(0)
{
    scan_iter = scan.end();
}

uint8_t Keyboard::get_latch()
{
    return latch;
}

bool Keyboard::get_tx_buf_empty() { return !tx_buf_count; }

extern WINDOW* msgWin;

/*
 * The VT100 triggers keyboard scans by setting bit (1<<6) in the status.
 * It then expects to receive one or more scan codes followed by an 0x7F
 * byte to indicate the end of the scan. Each byte takes 160 clocks to send.
 *
 * The real VT100 has a double buffer on it's UART, (like the keyboard) so
 * the transmit delay may be up to 320 clocks, however, direct xmit is
 * sufficient for the simulation.
 *
 * Setup and NoScroll are triggered as soon as they are received.
 *
 * The Shift and Control keys are state driven and on independent rows,
 * they do not count toward the three key maximum.
 *
 * Other keys must be NOT asserted for one scan then asserted for TWO scans
 * before they will be triggered. Upto three keys may be pressed at the same
 * time.
 *
 * The double scan and thee key maximum requirements prevent 'ghost'
 * keypresses trigged by the matrix being accepted.
 *
 * There is a 9 character buffer for transmitting data to the host; this must
 * have at least 3 bytes free. If it doesn't the keyboard lock is asserted.
 */

void Keyboard::set_status(uint8_t status)
{
  last_status = status;
    //printf("Got kbd status %02x at %04x\n",status,PC-ram); fflush(stdout);
    if ((status & (1<<6)) &&  state == KBD_IDLE) {
        //printf("SCAN START\n");fflush(stdout);
      //wprintw(msgWin,"Scan start\n");wrefresh(msgWin);
        state = KBD_SENDING;

	// NB: Time for the status to be received + time for the reply.
        clocks_until_next = 160+160;
    }

    if (status & 0x80) {
	if (!beeping) beep();
	if (tx_buf_count == 0) beeping = (beeping+1) % 48;
    } else beeping = 0;
    if (tx_buf_count == 0) tx_buf_count = 160;
}

void Keyboard::keypress(uint8_t keycode)
{
  //printf("PRESS %02x\n",keycode);fflush(stdout);
    keys.insert(keycode);
}

bool Keyboard::clock(bool rising)
{
    if (!rising) { return false; }
    if (tx_buf_count>0) tx_buf_count--;
    switch (state) {
    case KBD_IDLE:
        break;
    case KBD_SENDING:
        if (clocks_until_next == 0) {
	    if (keys.size() == 1 && last_sent.size() == 1 &&
		sending == KBD_SEND_2 &&
		scan.count(*(keys.begin())) == 0)  {

	      // Use keyboard rollover
	      scan = keys;
	      if (sending == KBD_SEND_1)
		scan.insert(last_sent.begin(),last_sent.end());

	      sending = KBD_SEND_1;
	      last_sent = keys;
	      keys.clear();

	    } else if (last_sent.size() != 0) {
	      if (sending == KBD_SEND_1) {
		sending = KBD_SEND_2;
	      } else {
		scan.clear();
		last_sent.clear();
		sending = KBD_IDLE;
	      }
	    } else if( keys.size() != 0) {
	      // We have sent an empty set, start next.
	      scan = keys;
	      last_sent = keys;
	      sending = KBD_SEND_1;
	      keys.clear();
	    } else {
	      // Nothing sending and nothing to send.
	      sending = KBD_IDLE;
	      scan.clear();
	      keys.clear();
	    }

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


