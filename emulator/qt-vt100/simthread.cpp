#include "simthread.h"
#include "8080/sim.h"
#include "8080/simglb.h"
#include <stdio.h>

extern "C" {
extern void int_on(void), int_off(void);
extern void init_io(void), exit_io(void);
};

SimThread::SimThread(QObject *parent) :
    QThread(parent)
{
}

void SimThread::run() {
    i_flag = 1;
    f_flag = 10;
    m_flag = 0;
    tmax = f_flag*10000;
    cpu = I8080;
    quint8 leds = 0;
    printf("\nRelease %s, %s\n", RELEASE, COPYR);
    if (f_flag > 0)
        printf("\nCPU speed is %d MHz\n", f_flag);
    else
        printf("\nCPU speed is unlimited\n");
#ifdef	USR_COM
    printf("\n%s Release %s, %s\n", USR_COM, USR_REL, USR_CPR);
#endif
    fflush(stdout);

    wrk_ram	= PC = ram;
    STACK = ram + 0xffff;
    if (cpu == I8080)	/* the unused flag bits are documented for */
        F = 2;		/* the 8080, so start with bit 1 set */
    memset((char *)	ram, m_flag, 65536);
    int_on();
    init_io();

    while (1) {
        msleep(500);
        leds = (leds+1)%8;
        emit outKbdStatus(leds);
    }

    exit_io();
    int_off();
}
