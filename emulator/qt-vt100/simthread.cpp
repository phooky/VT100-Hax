#include "simthread.h"

SimThread::SimThread(QObject *parent) :
    QThread(parent)
{
}

void SimThread::run() {
    quint8 leds = 0;
    while (1) {
        msleep(500);
        leds = (leds+1)%8;
        emit outKbdStatus(leds);
    }
}
