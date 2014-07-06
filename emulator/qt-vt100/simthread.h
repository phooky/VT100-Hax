#ifndef SIMTHREAD_H
#define SIMTHREAD_H

#include <QThread>

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
private:
    char* romPath;
signals:
    void outKbdStatus(quint8 status);
public slots:

};

extern SimThread* sim;

#endif // SIMTHREAD_H
