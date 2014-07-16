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
signals:
    void kbdKeypress(quint8 keycode);
public slots:
    void kbdStatus(quint8 kbdStatus);
    void updateDisplay();
    void updateMemory();
    void doSetup();
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
