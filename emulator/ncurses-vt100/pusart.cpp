#include "pusart.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <termios.h>

PUSART::PUSART() : 
  mode_select_mode(true),
  has_xmit_ready(true),
  mode(0),
  command(0),
  pty_fd(-1),
  has_rx_rdy(false)
{
  pty_fd = posix_openpt( O_RDWR | O_NOCTTY );
  unlockpt(pty_fd);
  int flags = fcntl(pty_fd, F_GETFL, 0);
  fcntl(pty_fd, F_SETFL, flags | O_NONBLOCK);

  struct termios  config;
  if(!isatty(pty_fd)) {}
  if(tcgetattr(pty_fd, &config) < 0) {}
  config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
		      INLCR | PARMRK | INPCK | ISTRIP | IXON);
  config.c_oflag = 0;
  config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
  config.c_cflag &= ~(CSIZE | PARENB);
  config.c_cflag |= CS8;
  config.c_cc[VMIN]  = 1;
  config.c_cc[VTIME] = 0;
  if(tcsetattr(pty_fd, TCSAFLUSH, &config) < 0) {}


}

bool PUSART::xmit_ready() { return has_xmit_ready; }

void PUSART::write_command(uint8_t cmd) {
  if (mode_select_mode) {
    mode_select_mode = false;
    mode = cmd; // like we give a wet turd
  } else {
    command = cmd;
    if (cmd & 1<<6) { // INTERNAL RESET
      mode_select_mode = true;
    }
    // Command 0x2f is BREAK
    // Command 0x2d is hangup
  }
}

void PUSART::write_data(uint8_t dat) {
  xoff = (dat == '\023');
  if (dat == '\023' || dat == '\021') return;
  write(pty_fd,&dat,1);
}

uint8_t PUSART::read_command() {
  /// always indicate data set ready
  bool tx_empty = false;
  bool sync_det = true;
  bool tx_rdy = true;
  bool rx_rdy = has_rx_rdy;
  return 0x80 | sync_det?0x40:0 | tx_empty?0x04:0 | rx_rdy?0x02:0 | tx_rdy?0x01:0;
}

bool PUSART::clock() {
  char c;
  if (has_rx_rdy || xoff) return false;
  int i = read(pty_fd,&c,1);
  if (i != -1) {
    data = c;
    has_rx_rdy = true;
    return true;
  }
  return false;
}

uint8_t PUSART::read_data() {
  has_rx_rdy = false;
  return data;
}

char* PUSART::pty_name() {
  return ptsname(pty_fd);
}
