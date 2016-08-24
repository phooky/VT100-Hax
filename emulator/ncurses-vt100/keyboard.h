#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <set>

typedef enum {
    KBD_IDLE =0,
    KBD_SENDING =1,
    KBD_RESPONDING =2,
    KBD_SEND_1 =3,
    KBD_SEND_2 =4,
} KbdState;

class Keyboard
{
public:
    Keyboard();
    uint8_t get_latch();
    bool get_tx_buf_empty();
private:
    KbdState state, sending;
    uint8_t latch;
    uint8_t last_status;
    uint8_t tx_buf_count;
    uint8_t beeping;
    std::set<uint8_t> keys;
    std::set<uint8_t> scan;
    std::set<uint8_t>::iterator scan_iter;
    uint32_t clocks_until_next;
    std::set<uint8_t> last_sent;
public:
    void set_status(uint8_t status);
    uint8_t get_status() { return last_status; }
    void keypress(uint8_t keycode);
    // Gets a clock for LBA4
    bool clock(bool rising); // return true if an interrupt is generated
    bool busy_scanning() { return !keys.empty(); }
};

#endif // KEYBOARD_H
