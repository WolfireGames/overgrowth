#!/bin/sh

run () {
  echo -n Running `$1 --version | sed q`...
  $* > /dev/null
  echo " done"
}


if test ! -f config/configure.ac ; then
  echo "$0: This script must be run from the Qhull top directory."
  exit 1
fi

echo -n Copying autoconf and automake files...
cp config/configure.ac .
cp config/Makefile-am-main Makefile.am
for d in html eg ; do
  cp config/Makefile-am-$d $d/Makefile.am
done
for d in libqhull ; do
  cp config/Makefile-am-$d src/$d/Makefile.am
done
echo -en ' done\nCopying program sources to src/libqhull...'
sources="src/qconvex/qconvex.c src/qdelaunay/qdelaun.c src/qhalf/qhalf.c \
  src/qhull/unix.c src/qvoronoi/qvoronoi.c src/rbox/rbox.c src/testqset/testqset.c"
cp $sources src/libqhull
echo " done"

run aclocal \
  && run libtoolize --force --copy \
  && run automake --foreign --add-missing --force-missing --copy \
  && run autoconf
