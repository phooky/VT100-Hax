#ifndef SIMTHREAD_H
#define SIMTHREAD_H

#include <QThread>
#include <QMutex>

#include "nvr.h"
#include "keyboard.h"

extern "C" {
#include "8080/sim.h"
}

class SimThread : public QThread
{
    Q_OBJECT
public:
    explicit SimThread(QObject *parent = 0,char* romPath = 0);
    void run();
    // Calls from C code
    BYTE ioIn(BYTE addr);
    void ioOut(BYTE addr, BYTE data);
    NVR nvr;
    Keyboard kbd;
private:
    char* romPath;
    quint32 stepsRemaining;
    QMutex simLock;
    QMutex kbdLock;
signals:
    void outKbdStatus(quint8 status);
    void cycleDone();
public slots:
    void simStep(quint32 count = 1);
    void simRun();
    void simStop();
    void keypress(quint8 keycode);
};

extern SimThread* sim;

#endif // SIMTHREAD_H
