# Locate SDL_net 2 library
# This module defines
# SDL2NET_LIBRARY, the name of the library to link against
# SDL2NET_FOUND, if false, do not try to link to SDL2
# SDL2NET_INCLUDE_DIR, where to find SDL_net.h
#

SET(SDL2NET_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

FIND_PATH(SDL2NET_INCLUDE_DIR SDL_net.h
	HINTS
	$ENV{SDL2NETDIR}
	PATH_SUFFIXES include/SDL2 include
	PATHS ${SDL2NET_SEARCH_PATHS}
)

FIND_LIBRARY(SDL2NET_LIBRARY
	NAMES SDL2_net
	HINTS
	$ENV{SDL2NETDIR}
	PATH_SUFFIXES lib64 lib
	PATHS ${SDL2NET_SEARCH_PATHS}
)

SET(SDL2NET_FOUND "YES")

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2 REQUIRED_VARS SDL2NET_LIBRARY SDL2NET_INCLUDE_DIR)
