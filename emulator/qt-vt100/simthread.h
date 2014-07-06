#ifndef SIMTHREAD_H
#define SIMTHREAD_H

#include <QThread>

class SimThread : public QThread
{
    Q_OBJECT
public:
    explicit SimThread(QObject *parent = 0,char* romPath = 0);
    void run();
private:
    char* romPath;
signals:
    void outKbdStatus(quint8 status);
public slots:

};

#endif // SIMTHREAD_H
