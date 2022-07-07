QT += core gui widgets

QTPLUGIN += qsvg

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    src/mainwindow.h

FORMS += \
    src/mainwindow.ui

TRANSLATIONS += \
    translations/FiveDChessServerAddressModifier_zh_CN.ts

RESOURCES += \
    images/license.svg

CONFIG += lrelease
CONFIG += embed_translations
CONFIG += debug_and_release

# Linux
release:DESTDIR = build/Release
release:OBJECTS_DIR = build/Release/.obj
release:MOC_DIR = build/Release/.moc
release:RCC_DIR = build/Release/.rcc
release:UI_DIR = build/Release/.ui

debug:DESTDIR = build/Debug
debug:OBJECTS_DIR = build/Debug/.obj
debug:MOC_DIR = build/Debug/.moc
debug:RCC_DIR = build/Debug/.rcc
debug:UI_DIR = build/Debug/.ui

# Windows
Release:DESTDIR = build/Release
Release:OBJECTS_DIR = build/Release/.obj
Release:MOC_DIR = build/Release/.moc
Release:RCC_DIR = build/Release/.rcc
Release:UI_DIR = build/Release/.ui

Debug:DESTDIR = build/Debug
Debug:OBJECTS_DIR = build/Debug/.obj
Debug:MOC_DIR = build/Debug/.moc
Debug:RCC_DIR = build/Debug/.rcc
Debug:UI_DIR = build/Debug/.ui

TARGET = 5DChessServerAddressModifier

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
