# Simple gcc Makefile for qhull and rbox (default gcc/g++)
#
#       see README.txt  
#       
# Results
#	qhull 	  Computes convex hulls and related structures
#       qconvex, qdelaunay, qhalf, qvoronoi
#                 Specializations of qhull for each geometric structure
#	user_eg	  An example of using shared library qhull6_p (with qh_QHpointer)
#       user_eg2  An example of using shared library qhull (without qh_QHpointer)
#       testqset  Standalone test of qset.c with mem.c
#       libqhullstatic.a Static library for qhull
#
# Make targets
#       make             Produce all of the results
#       make qhull_all   Produce qconvex, etc.
#                        Links program sources into src/libqhull
#       make qtest       Quick test of qset, rbox, and qhull
#       make doc         Print documentation
#       make install     Copy qhull, rbox, qhull.1, rbox.1 to BINDIR, MANDIR
#       make new         Rebuild qhull and rbox from source
#
#       make printall    Print all files
#       make clean       Remove object files
#       make cleanall    Remove generated files
#
#       BINDIR           directory where to copy executables
#       DESTDIR          destination directory
#       DOCDIR           directory where to copy html documentation
#       INCDIR           directory where to copy headers
#       LIBDIR           directory where to copy libraries
#       MANDIR           directory where to copy manual pages
#       PRINTMAN         command for printing manual pages
#       PRINTC           command for printing C files
#       CC               ANSI C or C++ compiler
#       CC_OPTS1         options used to compile .c files
#       CC_OPTS2         options used to link .o files
#       CC_OPTS3         options to build shared libraries
#
#       LIBQHULL_OBJS    .o files for linking
#       LIBQHULL_HDRS    .h files for printing
#       CFILES           .c files for printing
#       DOCFILES         documentation files
#       FILES            miscellaneous files for printing
#       TFILES           .txt versions of html files
#       FILES            all other files
#       LIBQHULL_OBJS    specifies the object files of libqhullstatic.a
#
# Do not replace tabs with spaces.  Needed by 'make' for build rules

# You may build the qhull programs without using a library
# make qhullx

DESTDIR = /usr/local
BINDIR	= $(DESTDIR)/bin
INCDIR	= $(DESTDIR)/include
LIBDIR	= $(DESTDIR)/lib
DOCDIR	= $(DESTDIR)/share/doc/qhull
MANDIR	= $(DESTDIR)/share/man/man1

# if you do not have enscript, try a2ps or just use lpr.  The files are text.
PRINTMAN = enscript -2rl
PRINTC = enscript -2r
# PRINTMAN = lpr
# PRINTC = lpr

#for Gnu's gcc compiler, -O2 for optimization, -g for debugging
CC        = gcc
CC_OPTS1  = -O2 -fPIC -ansi $(CC_WARNINGS)

# for Sun's cc compiler, -fast or O2 for optimization, -g for debugging, -Xc for ANSI
#CC       = cc
#CC_OPTS1 = -Xc -v -fast

# for Silicon Graphics cc compiler, -O2 for optimization, -g for debugging
#CC       = cc
#CC_OPTS1 = -ansi -O2

# for Next cc compiler with fat executable
#CC       = cc
#CC_OPTS1 = -ansi -O2 -arch m68k -arch i386 -arch hppa

# For loader, ld, 
CC_OPTS2 = $(CC_OPTS1)
CXX_OPTS2 = $(CXX_OPTS1)

# Default targets for make
     
all: qhull_links qhull_all qtest

clean:
	rm -f *.o 
	# Delete linked files from other directories [qhull_links]
	rm -f qconvex.c unix.c qdelaun.c qhalf.c qvoronoi.c rbox.c
	rm -f user_eg.c user_eg2.c testqset.c 
	
cleanall: clean
	rm -f qconvex qdelaunay qhalf qvoronoi qhull *.exe
	rm -f core user_eg user_eg2 testqset libqhullstatic.a

doc: 
	$(PRINTMAN) $(TXTFILES) $(DOCFILES)

install:
	mkdir -p $(BINDIR)
	mkdir -p $(DOCDIR)
	mkdir -p $(INCDIR)/libqhull
	mkdir -p $(MANDIR)
	cp -p qconvex qdelaunay qhalf qhull qvoronoi rbox $(BINDIR)
	cp -p libqhullstatic.a $(LIBDIR)
	cp -p ../../html/qhull.man $(MANDIR)/qhull.1
	cp -p ../../html/rbox.man $(MANDIR)/rbox.1
	cp -p ../../html/* $(DOCDIR)
	cp *.h $(INCDIR)/libqhull

new:	cleanall all

printall: doc printh printc printf

printh:
	$(PRINTC) $(LIBQHULL_HDRS)

printc:
	$(PRINTC) $(CFILES)

# LIBQHULL_OBJS_1 ordered by frequency of execution with small files at end.  Better locality.
# Same definitions as ../../Makefile

LIBQHULLS_OBJS_1= global.o stat.o geom2.o poly2.o merge.o \
        libqhull.o geom.o poly.o qset.o mem.o random.o 

LIBQHULLS_OBJS_2= $(LIBQHULLS_OBJS_1) usermem.o userprintf.o io.o user.o

LIBQHULLS_OBJS= $(LIBQHULLS_OBJS_2)  rboxlib.o userprintf_rbox.o

LIBQHULL_HDRS= user.h libqhull.h qhull_a.h geom.h \
        io.h mem.h merge.h poly.h random.h \
        qset.h stat.h

# CFILES ordered alphabetically after libqhull.c 
CFILES= ../qhull/unix.c libqhull.c geom.c geom2.c global.c io.c \
	mem.c merge.c poly.c poly2.c random.c rboxlib.c \
	qset.c stat.c user.c usermem.c userprintf.c \
	../qconvex/qconvex.c ../qdelaunay/qdelaun.c ../qhalf/qhalf.c ../qvoronoi/qvoronoi.c

TXTFILES= ../../Announce.txt ../../REGISTER.txt ../../COPYING.txt ../../README.txt ../Changes.txt
DOCFILES= ../../html/rbox.txt ../../html/qhull.txt

.c.o:
	$(CC) -c $(CC_OPTS1) -o $@ $<

# Work around problems with ../ in Red Hat Linux
qhull_links:
	[ -f qconvex.c ]  || ln -s ../qconvex/qconvex.c
	[ -f qdelaun.c ]  || ln -s ../qdelaunay/qdelaun.c
	[ -f qhalf.c ]    || ln -s ../qhalf/qhalf.c
	[ -f qvoronoi.c ] || ln -s ../qvoronoi/qvoronoi.c
	[ -f qhull.c ]    || ln -s ../qhull/unix.c
	[ -f rbox.c ]     || ln -s ../rbox/rbox.c
	[ -f user_eg.c ]     || ln -s ../user_eg/user_eg.c
	[ -f user_eg2.c ]     || ln -s ../user_eg2/user_eg2.c
	[ -f testqset.c ]     || ln -s ../testqset/testqset.c

# compile qhull without using bin/libqhullstatic.a
qhull_all: qconvex.o qdelaun.o qhalf.o qvoronoi.o unix.o user_eg.o user_eg2.o rbox.o testqset.o $(LIBQHULLS_OBJS)
	$(CC) -o qconvex $(CC_OPTS2) -lm $(LIBQHULLS_OBJS_2) qconvex.o 
	$(CC) -o qdelaunay $(CC_OPTS2) -lm $(LIBQHULLS_OBJS_2) qdelaun.o
	$(CC) -o qhalf $(CC_OPTS2) -lm $(LIBQHULLS_OBJS_2) qhalf.o 
	$(CC) -o qvoronoi $(CC_OPTS2) -lm $(LIBQHULLS_OBJS_2) qvoronoi.o 
	$(CC) -o qhull $(CC_OPTS2) -lm $(LIBQHULLS_OBJS_2) unix.o 
	$(CC) -o rbox $(CC_OPTS2) -lm rbox.o rboxlib.o random.o usermem.o userprintf_rbox.o
	$(CC) -o user_eg $(CC_OPTS2) -lm $(LIBQHULLS_OBJS_2) user_eg.o 
	$(CC) -o user_eg2 $(CC_OPTS2) -lm $(LIBQHULLS_OBJS_1) user_eg2.o  usermem.o userprintf.o io.o
	$(CC) -o testqset $(CC_OPTS2) -lm mem.o qset.o testqset.o
	-ar -rs libqhullstatic.a $(LIBQHULLS_OBJS)
	#libqhullstatic.a is not needed for qhull
	#If 'ar -rs' fails try using 'ar -s' with 'ranlib'
	#ranlib libqhullstatic.a

qtest:
	@echo Testing qset.c and mem.c with testqset
	./testqset 10000
	@echo Run the qhull smoketest
	./rbox D4 | ./qhull
	
# end of Makefile
