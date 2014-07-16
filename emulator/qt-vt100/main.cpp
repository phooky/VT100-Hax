#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include "simthread.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    sim = new SimThread(&a,argv[1]);
    sim->start();
    QObject::connect(sim,SIGNAL(outKbdStatus(quint8)),&w,SLOT(kbdStatus(quint8)));
    QObject::connect(sim,SIGNAL(cycleDone()),&w,SLOT(updateMemory()));

    w.connectSim(sim);
    a.exec();
}
