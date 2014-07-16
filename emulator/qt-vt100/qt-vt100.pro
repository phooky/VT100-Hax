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
    simthread.cpp \
    8080/sim1.c \
    8080/sim1a.c \
    8080/sim2.c \
    8080/sim3.c \
    8080/sim4.c \
    8080/sim5.c \
    8080/sim6.c \
    8080/sim7.c \
    8080/simfun.c \
    8080/simglb.c \
    8080/simint.c \
    nvr.cpp \
    keyboard.cpp

HEADERS  += mainwindow.h \
    simthread.h \
    8080/sim.h \
    8080/simglb.h \
    nvr.h \
    keyboard.h

FORMS    += mainwindow.ui
