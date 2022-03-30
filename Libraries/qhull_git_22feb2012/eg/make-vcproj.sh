#!/bin/bash
#
# make-vcproj.sh -- Make sln and vcproj files from CMakeLists.txt and qhull-all.pro
#
# [jan'12] Additional changes made by hand for qhull 2012.1.   This file updated but not tested.
#
# $Id: //main/2011/qhull/eg/make-vcproj.sh#5 $$Change: 1476 $
# $DateTime: 2012/01/28 09:37:56 $$Author: bbarber $

if [[ ! -f CMakeLists.txt || ! -f src/qhull-all.pro ]]; then
    echo "Excute eg/make-vcproj.sh from qhull directory with CMakeLists.txt and src/qhull-all.pro"
    exit
fi

echo "Set up build directories..."
if [[ ! -d build-prev ]]; then
    echo "Backup previous build"
    cp -r build build-prev && rm -r build
fi
rm -r buildvc buildqt
mkdir -p build
mkdir -p buildvc
mkdir -p buildqt
echo "Create vcproj files with cmake and qmake..."
cd buildvc && cmake -G "Visual Studio 8 2005" .. && cmake ..
cd ..
cd buildqt && qmake -tp vc -r ../src/qhull-all.pro
cd ..
if [[ ! -f CMakeLists.txt || ! -f buildvc/qhull.vcproj || ! -f buildqt/qhulltest/qhulltest.vcproj ]]; then
    echo "qmake and cmake did not build vcproj files in buildvc/ and buildqt"
    exit
fi


for f in buildvc/*.vcproj buildqt/qhulltest/*.vcproj; do 
    echo $f
    if [[ ! ${f##*INSTALL*} || ! ${f##*ALL_BUILD*} || ! ${f##*ZERO_CHECK*}  ]]; then
	continue
    fi
    dest=build/${f##*\/}
    sed -r -e 's|[cC]:\\bash\\local\\qhull|..|g' \
    	-e 's|[cC]:/bash/local/qhull|..|g' \
    	-e '\|CMake| d' \
    	-e 's/;CMAKE_INTDIR=..quot;[A-Za-z]*..quot;//' \
    	-e 's/LinkIncremental="2"/LinkIncremental="1"/' \
    	-e 's/RuntimeLibrary[=]/RuntimeTypeInfo="false"  RuntimeLibrary=/' \
    	-e 's/.*RuntimeTypeInfo."TRUE".*//' \
    	-e 's/buildvc/build/g' \
    	-e 's/buildqt/build/g' \
    	-e 's/\.\.\\\.\./../g' \
    	-e 's|\.\./\.\.|..|g' \
    	-e 's/c:\\qt\\[0-9]\.[0-9]\.[0-9]/\$(QTDIR)/g' \
    	-e 's|..\\build\\[a-zA-Z]*[\\/]([_a-z0-9]*.pdb)|..\\bin\\\1|g' \
    	-e 's|..\\build\\[a-zA-Z]*[\\/]([_a-z0-9]*.exe)|..\\bin\\\1|g' \
    	-e 's|..\\build\\[a-zA-Z]*[\\/]([_a-z0-9]*.lib)|..\\lib\\\1|g' \
    	-e 's|..\\build\\[a-zA-Z]*[\\/]([_a-z0-9]*.dll)|..\\bin\\\1|g' \
    	-e 's| [a-zA-Z]*[\\/]([_a-z0-9]*.lib)| ..\\lib\\\1|g' \
    	-e 's/"([_a-z0-9]*.exe)/"..\\bin\\\1/g' \
	$f > $dest
done

echo -e '\nExcept for qhulltest.vcproj,\n*.vcproj: delete empty File in Files section near end of vcproj'
echo -e '\nExcept for qhulltest.vcproj,\n*.vcproj: rename debug targets to qhull_d.dll, etc.'

# If need to rebuild sln
sed -e '\|Project.*ALL_BUILD|,\|EndProject$| d' \
    -e '\|Project.*INSTALL|,\|EndProject$| d' \
    -e '\|Project.*ZERO_CHECK|,\|EndProject$| d' \
    buildvc/qhull.sln >build/qhull.sln

echo
echo 'qhull.sln: Remove first line of each GUID list -- postProject\n.* > postProject'
echo 'qhull.sln: Remove four empty project sections'
echo 'qhull.sln: Remove first part of Global section.'
echo 'qhull.sln: Save as PC'
echo '*.vcproj: Remove libqhullcpp from AdditionalIncludeDirectories except for cpp'
echo
echo 'Open qhull.sln with DevStudio and add qhulltest.vcproj'
echo 'Add dependencies on libqhull libqhull6_p and qhullcpp'
echo 'Change Linker>OutputFile to qhulltest.exe'
echo 'Remove qhulltest via Configuration Manager from Release, Debug, etc.'


