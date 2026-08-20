QT += widgets core gui network serialport charts

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    chartmanager.cpp \
    main.cpp \
    mainwindow.cpp \
    serialmanager.cpp \
    settingsmanager.cpp \
    tcpstreamserver.cpp

HEADERS += \
    chartmanager.h \
    logger.h \
    mainwindow.h \
    serialmanager.h \
    settingsmanager.h \
    tcpstreamserver.h

FORMS += \
    mainwindow.ui

TARGET = SmartMonitoringServer

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
