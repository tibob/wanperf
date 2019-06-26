#-------------------------------------------------
#
# Project created by QtCreator 2016-05-05T21:00:07
#
#-------------------------------------------------

QMAKE_CXXFLAGS  += -std=c++11
QT              += core gui network
QT              += websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = wanperf
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    udpsender.cpp \
    networkmodel.cpp \
    wsclient.cpp \
    udpsenderlistmodel.cpp

HEADERS  += mainwindow.h \
    udpsender.h \
    networkmodel.h \
    wsclient.h \
    udpsenderlistmodel.h

FORMS    += mainwindow.ui

DISTFILES += \
    todo.txt
