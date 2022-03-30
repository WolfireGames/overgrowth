# -------------------------------------------------
# libqhullcpp.pro -- Qt project for Qhull cpp shared library
# -------------------------------------------------

include(../qhull-warn.pri)

DESTDIR = ../../lib
TEMPLATE = lib
CONFIG += staticlib warn_on
CONFIG -= qt rtti
build_pass:CONFIG(debug, debug|release):{
   TARGET = qhullcpp_d
   OBJECTS_DIR = Debug
}else:build_pass:CONFIG(release, debug|release):{
   TARGET = qhullcpp
   OBJECTS_DIR = Release
}
MOC_DIR = moc

DEFINES += qh_QHpointer # libqhull/user.h

INCLUDEPATH += ../../src
INCLUDEPATH += $$PWD # for MOC_DIR

CONFIG += qhull_warn_shadow qhull_warn_conversion

SOURCES += Coordinates.cpp
SOURCES += PointCoordinates.cpp
SOURCES += Qhull.cpp
SOURCES += QhullFacet.cpp
SOURCES += QhullFacetList.cpp
SOURCES += QhullFacetSet.cpp
SOURCES += QhullHyperplane.cpp
SOURCES += QhullPoint.cpp
SOURCES += QhullPoints.cpp
SOURCES += QhullPointSet.cpp
SOURCES += QhullQh.cpp
SOURCES += QhullRidge.cpp
SOURCES += QhullSet.cpp
SOURCES += QhullStat.cpp
SOURCES += QhullVertex.cpp
SOURCES += QhullVertexSet.cpp
SOURCES += RboxPoints.cpp
SOURCES += RoadError.cpp
SOURCES += RoadLogEvent.cpp
SOURCES += UsingLibQhull.cpp

HEADERS += Coordinates.h
HEADERS += functionObjects.h
HEADERS += PointCoordinates.h
HEADERS += Qhull.h
HEADERS += QhullError.h
HEADERS += QhullFacet.h
HEADERS += QhullFacetList.h
HEADERS += QhullFacetSet.h
HEADERS += QhullHyperplane.h
HEADERS += QhullIterator.h
HEADERS += QhullLinkedList.h
HEADERS += QhullPoint.h
HEADERS += QhullPoints.h
HEADERS += QhullPointSet.h
HEADERS += QhullQh.h
HEADERS += QhullRidge.h
HEADERS += QhullSet.h
HEADERS += QhullSets.h
HEADERS += QhullStat.h
HEADERS += QhullVertex.h
HEADERS += RboxPoints.h
HEADERS += RoadError.h
HEADERS += RoadLogEvent.h
HEADERS += UsingLibQhull.h
