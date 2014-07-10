#include "nvr.h"
#include <stdio.h>
enum {
    STANDBY = 0b111,
    ACCEPT_ADDR = 0b001,
    ERASE = 0b101,
    ACCEPT_DATA = 0b000,
    WRITE = 0b100,
    READ = 0b110,
    SHIFT_OUT = 0b010,
    UNUSED = 0b011
};

NVR::NVR() : address_reg(0xffff), data_reg(0), latch(STANDBY << 1)
{
    for (quint8 idx = 0; idx < 100; idx++) {
        contents[idx] = 0;
    }
}

void NVR::set_latch(quint8 latch) {
    this->latch = latch;
}

quint8 compute_addr(quint32 address_reg) {
    address_reg = ~address_reg;
    quint8 tens = 0;
    for (quint8 i = 0; i < 10; i++) {
        if (address_reg & 0x01) tens = 9-i;
        address_reg >>= 1;
    }
    quint8 ones = 0;
    for (quint8 i = 0; i < 10; i++) {
        if (address_reg & 0x01) ones = 9-i;
        address_reg >>= 1;
    }
    return tens*10 + ones;
}

void NVR::clock() {
    quint8 command = (latch>>1) & 0b111;
    quint8 bit = latch & 0x01;
    switch(command) {
    case STANDBY:
        break;
    case ACCEPT_ADDR:
        address_reg = (address_reg << 1) | bit;
        break;
    case ERASE:
        contents[compute_addr(address_reg)] = 0;
        break;
    case ACCEPT_DATA:
        data_reg = (data_reg << 1) | bit;
        break;
    case WRITE:
        contents[compute_addr(address_reg)] = data_reg & 0x3fff;
        break;
    case READ:
        printf("reading %x:%02d\n",address_reg,compute_addr(address_reg));
        fflush(stdout);
        data_reg = contents[compute_addr(address_reg)];
        break;
    case SHIFT_OUT:
        out = data_reg & 0x0200;
        data_reg <<= 1;
        break;
    case UNUSED:
        break;
    }
}

bool NVR::output() {
    return out;
}

