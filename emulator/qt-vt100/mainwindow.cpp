#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "8080/sim.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->updateButton,SIGNAL(clicked()),this,SLOT(updateDisplay()));
}

void MainWindow::connectSim(SimThread *sim)
{
    QObject::connect(ui->startButton,SIGNAL(released()),sim,SLOT(simRun()));
    QObject::connect(ui->stopButton,SIGNAL(released()),sim,SLOT(simStop()));
    QObject::connect(ui->stepButton,SIGNAL(released()),sim,SLOT(simStep()));
    QObject::connect(ui->setupButton,SIGNAL(clicked()),sim,SLOT(doSetup()));
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

void MainWindow::updateDisplay()
{
    ui->screen->clear();
    ui->screen->update();
}
