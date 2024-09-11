QT = core

CONFIG += c++11 cmdline


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(RTKLib.pri)
CONFIG -= debug_and_release

SOURCES += \
        HASPPP.cpp \
        main.cpp
win32{
QMAKE_LFLAGS += -Wl,--stack,32000000

}



HEADERS += \
    HASPPP.h \
    rtklib.h
