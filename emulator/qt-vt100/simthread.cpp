
#include "simthread.h"
#include <QFile>
#include "8080/sim.h"
#include "8080/simglb.h"
#include <stdio.h>

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

SimThread* sim;

SimThread::SimThread(QObject *parent,char* romPath) :
    QThread(parent), stepsRemaining(0)
{
    this->romPath = romPath;
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

void SimThread::run() {
    i_flag = 1;
    f_flag = 10;
    m_flag = 0;
    tmax = f_flag*10000;
    cpu = I8080;
    printf("\nRelease %s, %s\n", RELEASE, COPYR);
    if (f_flag > 0)
        printf("\nCPU speed is %d MHz\n", f_flag);
    else
        printf("\nCPU speed is unlimited\n");
#ifdef	USR_COM
    printf("\n%s Release %s, %s\n", USR_COM, USR_REL, USR_CPR);
#endif
    fflush(stdout);

    printf("Prep ram\n");
    fflush(stdout);
    wrk_ram	= PC = ram;
    STACK = ram + 0xffff;
    if (cpu == I8080)	/* the unused flag bits are documented for */
        F = 2;		/* the 8080, so start with bit 1 set */
    memset((char *)	ram, m_flag, 65536);
    // load binary
    printf("Loading rom %s...\n",romPath);
    fflush(stdout);
    QFile romFile(romPath);
    if (!romFile.open(QIODevice::ReadOnly)) {
        printf("Failed to read rom file\n");
        fflush(stdout);
        return;
    }
    qint64 count = romFile.read((char*)ram,65536);
    printf("Read ROM file; %lld bytes\n",count);
    romFile.close();
    int_on();
    // add local io hooks

    i_flag = 0;

    // We are always running the CPU in single-step mode so we can do the clock toggles when necessary.
    cpu_state = SINGLE_STEP;
    while (1) {
        simLock.lock();
        if (stepsRemaining > 0) {
            stepsRemaining--;
            simLock.unlock();
            const uint32_t start = t_ticks;
            cpu_error = NONE;
            cpu_8080();
            int_data = 0xff;
            const uint16_t t = t_ticks - start;
            if (lba4.add_ticks(t)) {
                kbdLock.lock();
                if (kbd.clock(lba4.get_value())) {
                        int_data &= 0xcf;
                        int_int = 1;
                }
                kbdLock.unlock();
            }
            if (lba7.add_ticks(t)) {
                nvr.clock(lba7.get_value());
            }
            if (vertical.add_ticks(t)) {
                if (vertical.get_value()) {
                    int_data &= 0xe7;
                }
            }
            // Compute clocks: LBA7, LBA4, Even Signal, Vertical interrupt
            // LBA7 goes to the NVR clock
            // LBA4 goes to the keyboard
            // Vertical Freq generates an interrupt
            // In terms of processor cycles:
            // LBA4 : period of 22 cycles
            // LBA7 : period of 182 cycles
            // Vertical interrupt: period of 46084 cycles
        } else {
            simLock.unlock();
            msleep(50);
        }
    }
    /*
    if (cpu_error == OPHALT)
        if (handel_break())
            if (!cpu_error)
                goto cont;
    cpu_err_msg();
    print_head();
    print_reg();
    */

    int_off();
}

BYTE SimThread::ioIn(BYTE addr) {
    if (addr == 0x42) {
        // Read buffer flag
        quint8 flags = 0x02;
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
        printf(" IN PORT %02x at %04lx\n",addr,PC-ram);fflush(stdout);
    }
    return 0;
}

void SimThread::ioOut(BYTE addr, BYTE data) {
    switch(addr) {
    case 0x82:
        //printf("OUT PORT %02x <- %02x\n",addr,data);
        //fflush(stdout);
        kbd.set_status(data);
        emit outKbdStatus(data);
        break;
    case 0x62:
        //printf("NVRAM %02x\n",data);
        //fflush(stdout);
        nvr.set_latch(data);
        break;
    default:
        printf("OUT PORT %02x <- %02x\n",addr,data);fflush(stdout);
        break;
    }
}

void SimThread::simStep(quint32 count)
{
    simLock.lock();
    stepsRemaining = count;
    printf("Current PC is %04x; executing %d instructions\n",(unsigned int)(PC-ram),count); fflush(stdout);
    simLock.unlock();
}

void SimThread::simRun()
{
    printf("START******************\n"); fflush(stdout);
    simLock.lock();
    stepsRemaining = 0xffffffff;
    simLock.unlock();
}

void SimThread::simStop()
{
    printf("STOP*******************\n"); fflush(stdout);
    simLock.lock();
    printf("Current PC is %04x\n",(unsigned int)(PC-ram)); fflush(stdout);
    stepsRemaining = 1;
    simLock.unlock();
}

void SimThread::keypress(quint8 keycode)
{
    kbdLock.lock();
    kbd.keypress(keycode);
    kbdLock.unlock();
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
