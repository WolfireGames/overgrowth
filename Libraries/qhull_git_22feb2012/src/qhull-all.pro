# -------------------------------------------------
# qhull-all.pro -- Qt project to build executables and static libraries
#
# To build with Qt on mingw
#   Download Qt SDK, install Perl
#   /c/qt/2010.05/qt> ./configure -static -platform win32-g++ -fast -no-qt3support
#
# To build DevStudio sln and proj files (Qhull ships with cmake derived files)
# qmake is in Qt's bin directory
# ../build> qmake -tp vc -r ../src/qhull-all.pro
# Need to add Build Dependencies, disable rtti, rename targets to qhull.dll, qhull6_p.dll and qhull6_pd.dll
# -------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += libqhull        #shared library
SUBDIRS += libqhullp       #shared library with qh_QHpointer (libqhull/user.h)
SUBDIRS += user_eg         #user program linked to libqhull6_p (libqhullp)

SUBDIRS += libqhullstatic  #static library
SUBDIRS += libqhullstaticp #static library with qh_QHpointer
SUBDIRS += qhull           #qhull programs linked to libqhullstatic
SUBDIRS += qconvex
SUBDIRS += qdelaunay
SUBDIRS += qhalf
SUBDIRS += qvoronoi
SUBDIRS += rbox
SUBDIRS += user_eg2        #user program linked to libqhull
SUBDIRS += testqset        #test program for qset.c with mem.c

SUBDIRS += libqhullcpp     #static library for C++ interface with libqhullstaticp
SUBDIRS += user_eg3        #user program with libqhullcpp and libqhullstaticp
SUBDIRS += qhulltest       #test program with Qt, libqhullcpp, and libqhullstaticp

OTHER_FILES += Changes.txt
OTHER_FILES += CMakeLists.txt
OTHER_FILES += Make-config.sh
OTHER_FILES += ../Announce.txt
OTHER_FILES += ../CMakeLists.txt
OTHER_FILES += ../COPYING.txt
OTHER_FILES += ../File_id.diz
OTHER_FILES += ../index.htm
OTHER_FILES += ../Makefile
OTHER_FILES += ../README.txt
OTHER_FILES += ../REGISTER.txt
OTHER_FILES += ../eg/q_eg
OTHER_FILES += ../eg/q_egtest
OTHER_FILES += ../eg/q_test
OTHER_FILES += ../html/index.htm
OTHER_FILES += ../html/qconvex.htm
OTHER_FILES += ../html/qdelau_f.htm
OTHER_FILES += ../html/qdelaun.htm
OTHER_FILES += ../html/qhalf.htm
OTHER_FILES += ../html/qh-code.htm
OTHER_FILES += ../html/qh-eg.htm
OTHER_FILES += ../html/qh-faq.htm
OTHER_FILES += ../html/qh-get.htm
OTHER_FILES += ../html/qh-impre.htm
OTHER_FILES += ../html/qh-optc.htm
OTHER_FILES += ../html/qh-optf.htm
OTHER_FILES += ../html/qh-optg.htm
OTHER_FILES += ../html/qh-opto.htm
OTHER_FILES += ../html/qh-optp.htm
OTHER_FILES += ../html/qh-optq.htm
OTHER_FILES += ../html/qh-optt.htm
OTHER_FILES += ../html/qh-quick.htm
OTHER_FILES += ../html/qhull.htm
OTHER_FILES += ../html/qhull.man
OTHER_FILES += ../html/qhull.txt
OTHER_FILES += ../html/qhull-cpp.xml
OTHER_FILES += ../html/qvoron_f.htm
OTHER_FILES += ../html/qvoronoi.htm
OTHER_FILES += ../html/rbox.htm
OTHER_FILES += ../html/rbox.man
OTHER_FILES += ../html/rbox.txt
