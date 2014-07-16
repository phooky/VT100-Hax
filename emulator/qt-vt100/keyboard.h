#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <list>

typedef enum {
    KBD_IDLE =0,
    KBD_SENDING =1,
    KBD_RESPONDING =2,
} KbdState;

class Keyboard
{
public:
    Keyboard();
    uint8_t get_latch();
    bool get_tx_buf_empty();
private:
    KbdState state;
    uint8_t latch;
    bool tx_buf_empty;
    std::list<uint8_t> keys;
    std::list<uint8_t> scan;
    std::list<uint8_t>::iterator scan_iter;
    uint32_t clocks_until_next;
public:
    void set_status(uint8_t status);
    void keypress(uint8_t keycode);
    // Gets a clock for LBA4
    bool clock(bool rising); // return true if an interrupt is generated
};

#endif // KEYBOARD_H
