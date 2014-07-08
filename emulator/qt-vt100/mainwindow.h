#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "simthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void connectSim(SimThread* sim);
    ~MainWindow();
public slots:
    void kbdStatus(quint8 kbdStatus);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
