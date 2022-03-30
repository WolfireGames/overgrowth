# -------------------------------------------------
# qhulltest.pro -- Qt project for qhulltest.exe (QTestLib)
# -------------------------------------------------

include(../qhull-app-cpp.pri)

TARGET = qhulltest
CONFIG += qtestlib
MOC_DIR = moc
INCLUDEPATH += ..  # for MOC_DIR

PRECOMPILED_HEADER = RoadTest.h

SOURCES += ../libqhullcpp/qt-qhull.cpp
SOURCES += Coordinates_test.cpp
SOURCES += PointCoordinates_test.cpp
SOURCES += Qhull_test.cpp
SOURCES += QhullFacet_test.cpp
SOURCES += QhullFacetList_test.cpp
SOURCES += QhullFacetSet_test.cpp
SOURCES += QhullHyperplane_test.cpp
SOURCES += QhullLinkedList_test.cpp
SOURCES += QhullPoint_test.cpp
SOURCES += QhullPoints_test.cpp
SOURCES += QhullPointSet_test.cpp
SOURCES += QhullRidge_test.cpp
SOURCES += QhullSet_test.cpp
SOURCES += qhulltest.cpp
SOURCES += QhullVertex_test.cpp
SOURCES += RboxPoints_test.cpp
SOURCES += RoadTest.cpp
SOURCES += UsingLibQhull_test.cpp

HEADERS += RoadTest.h
