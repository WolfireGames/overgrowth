# -------------------------------------------------
# libqhull.pro -- Qt project for Qhull shared library with qh_QHpointer
#   Built with qh_QHpointer=1
# -------------------------------------------------

include(../qhull-warn.pri)
include(../qhull-libqhull-src.pri)

DESTDIR = ../../lib
DLLDESTDIR = ../../bin
TEMPLATE = lib
CONFIG += shared warn_on
CONFIG -= qt

build_pass:CONFIG(debug, debug|release):{
    TARGET = qhull_pd
    OBJECTS_DIR = Debug
}else:build_pass:CONFIG(release, debug|release):{
    TARGET = qhull_p
    OBJECTS_DIR = Release
}
win32-msvc* : QMAKE_LFLAGS += /INCREMENTAL:NO
win32-msvc* : DEF_FILE += ../../src/libqhullp/qhull_p-exports.def

DEFINES += qh_QHpointer # libqhull/user.h
