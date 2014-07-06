#-------------------------------------------------
#
# Project created by QtCreator 2014-07-05T10:07:30
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt-vt100
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    simthread.cpp

HEADERS  += mainwindow.h \
    simthread.h

FORMS    += mainwindow.ui
