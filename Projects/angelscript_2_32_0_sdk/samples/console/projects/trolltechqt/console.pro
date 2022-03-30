TEMPLATE = app

SOURCES += ../../source/main.cpp

DESTDIR = ../../bin

INCLUDEPATH += ../../../../angelscript/include

LIBS += -L../../../../angelscript/lib -langelscript

win32: LIBS += -lwinmm

CONFIG -= debug debug_and_release release app_bundle qt dll

CONFIG += console thread release

DEFINES += _CRT_SECURE_NO_WARNINGS
