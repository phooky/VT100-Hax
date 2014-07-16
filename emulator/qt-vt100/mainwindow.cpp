#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "8080/sim.h"
#include "8080/simglb.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->updateButton,SIGNAL(clicked()),this,SLOT(updateDisplay()));
}

void MainWindow::connectSim(SimThread *sim)
{
    QObject::connect(ui->startButton,SIGNAL(clicked()),sim,SLOT(simRun()));
    QObject::connect(ui->stopButton,SIGNAL(clicked()),sim,SLOT(simStop()));
    QObject::connect(ui->stepButton,SIGNAL(clicked()),sim,SLOT(simStep()));
    QObject::connect(ui->stepButton,SIGNAL(clicked()),this,SLOT(updateMemory()));
    QObject::connect(ui->setupButton,SIGNAL(clicked()),this,SLOT(doSetup()));
    QObject::connect(this,SIGNAL(kbdKeypress(quint8)),sim,SLOT(keypress(quint8)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::kbdStatus(quint8 kbdStatus)
{
    ui->user1LED->setChecked(kbdStatus & 0x01);
    ui->user2LED->setChecked(kbdStatus & 0x02);
    ui->user3LED->setChecked(kbdStatus & 0x04);
    ui->user4LED->setChecked(kbdStatus & 0x08);
}

#include <stdio.h>

void MainWindow::updateDisplay()
{
    ui->screen->clear();
    quint16 start = 0x2000;
    for (quint8 i = 0; i < 100; i++) {
        QString line;
        //printf("%04x\n",start);fflush(stdout);
        char* p = (char*)ram + start;
        char* maxp = p + 132;
        while (*p != 0x7f && p != maxp) {
            unsigned char c = *(p++);
            if (c != 0) {
                //printf("%c (%d)\n",c,c);fflush(stdout);
                line.append(QChar(c));
            }
        }
        if (p == maxp) break;
        // at terminator
        p++;
        unsigned char a1 = *(p++);
        unsigned char a2 = *(p++);
        //printf("Next: %02x %02x\n",a1,a2);fflush(stdout);
        quint16 next = (((a1&0x10)!=0)?0x2000:0x4000) | ((a1&0x0f)<<8) | a2;
        ui->screen->append(line);
        if (start == next) break;
        start = next;
    }
    ui->screen->update();
    updateMemory();
}

void MainWindow::updateMemory()
{
    ui->memory->clear();
    for (quint16 offset = 0x2000; offset < 0x22C0; offset += 0x20) {
        QString line = QString("%1: ").arg(offset,4,16,QChar('0'));
        for (int i = 0; i < 0x20; i++) {
            line.append(QString("%1 ").arg(ram[offset+i],2,16,QChar('0')));
        }
        ui->memory->append(line);
    }
    ui->memory->update();
}

void MainWindow::doSetup()
{
    emit kbdKeypress(0x7b);
}
