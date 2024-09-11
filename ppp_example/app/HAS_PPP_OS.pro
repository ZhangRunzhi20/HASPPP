QT += core
QT -= gui

CONFIG += c++11

TARGET = HAS_PPP_OS
CONFIG += console
CONFIG -= app_bundle


include(RTKLib.pri)
TEMPLATE = app

SOURCES += main.cpp \
    Src/config.c \
    postMain.cpp \
    QPPPPost.cpp \
    Src/ephemeris.c \
    Src/geoid.c \
    Src/ionex.c \
    Src/lambda.c \
    Src/pntpos.c \
    Src/postpos.c \
    Src/ppp.c \
    Src/ppp_ar.c \
    Src/preceph.c \
    Src/rinex.c \
    Src/rtcm.c \
    Src/rtcm2.c \
    Src/rtcm3.c \
    Src/rtcm3e.c \
    Src/rtkcmn.c \
    Src/rtkpos.c \
    Src/sbas.c \
    Src/solution.c \
    Src/tides.c \
    HASPPP.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    postMain.h \
    QPPPPost.h \
    HASPPP.h \
    rtklib.h
win32{
LIBS +=  -lwinmm
CONFIG -= debug_and_release
QMAKE_LFLAGS += -Wl,--stack,32000000
CONFIG += unicode
}



