# -------------------------------------------------
# qhull-app-sharedp.pri -- Qt include project for C qhull applications linked with libqhull6_p 
#   Compile code with -Dqh_QHpointer 
# -------------------------------------------------

include(qhull-warn.pri)

DESTDIR = ../../bin
TEMPLATE = app
CONFIG += console warn_on
CONFIG -= qt

LIBS += -L../../lib
build_pass:CONFIG(debug, debug|release){
   LIBS += -lqhull_pd
   OBJECTS_DIR = Debug
}else:build_pass:CONFIG(release, debug|release){
   LIBS += -lqhull_p
   OBJECTS_DIR = Release
}
win32-msvc* : QMAKE_LFLAGS += /INCREMENTAL:NO

DEFINES += qh_QHpointer # libqhull/user.h
win32-msvc* : DEFINES += qh_QHpointer_dllimport # libqhull/user.h

INCLUDEPATH += ../libqhull
CONFIG += qhull_warn_conversion


