QT       += core gui network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET=UIConnect
CONFIG += c++17
RC_ICONS = network-256.ico
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    direwolfconnect.cpp \
    uikissutils.cpp

HEADERS += \
    direwolfconnect.h \
    maidenhead.h \
    uikissutils.h

FORMS += \
    direwolfconnect.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    README.md \
    network-256.ico
