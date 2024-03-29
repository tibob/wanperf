#-------------------------------------------------
#
# Project created by QtCreator 2016-05-05T21:00:07
#
#-------------------------------------------------

QMAKE_CXXFLAGS  += -std=c++17
QT              += core gui network
QT              += websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = wanperf
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    networklayer.cpp \
    networklayerlistmodel.cpp \
    udpsender.cpp \
    networkmodel.cpp \
    udpsenderlistmodel.cpp \
    udpsenderthread.cpp

HEADERS  += mainwindow.h \
    networklayer.h \
    networklayerlistmodel.h \
    udpsender.h \
    networkmodel.h \
    udpsenderlistmodel.h \
    udpsenderthread.h

FORMS    += mainwindow.ui

DISTFILES += \
    LICENSE.txt \
    README.md
