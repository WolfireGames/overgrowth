


IF (WIN32)

	# Maya plugins can only be compiled with msvc
	IF (MSVC)

		FIND_PATH(MAYA_INCLUDE_PATH maya/MTypes.h
			PATHS
				"$ENV{PROGRAMFILES}/Autodesk/Maya8.5/include"
				"$ENV{MAYA_LOCATION}/include"
			DOC "The directory where MTypes.h resides")

		# Find maya version!

		FIND_LIBRARY(MAYA_FOUNDATION_LIBRARY Foundation
			PATHS
				"$ENV{PROGRAMFILES}/Autodesk/Maya8.5/lib"
				"$ENV{MAYA_LOCATION}/lib"
			DOC "The directory where Foundation.lib resides")

		FIND_LIBRARY(MAYA_OPENMAYA_LIBRARY OpenMaya
			PATHS
				"$ENV{PROGRAMFILES}/Autodesk/Maya8.5/lib"
				"$ENV{MAYA_LOCATION}/lib"
			DOC "The directory where OpenMaya.lib resides")

		FIND_LIBRARY(MAYA_OPENMAYAANIM_LIBRARY OpenMayaAnim
			PATHS
				"$ENV{PROGRAMFILES}/Autodesk/Maya8.5/lib"
				"$ENV{MAYA_LOCATION}/lib"
			DOC "The directory where OpenMayaAnim.lib resides")

		SET(MAYA_LIBRARIES 
			${MAYA_FOUNDATION_LIBRARY}
			${MAYA_OPENMAYA_LIBRARY}
			${MAYA_OPENMAYAANIM_LIBRARY})

		SET(MAYA_EXTENSION ".mll")

	ENDIF (MSVC)
ELSE (WIN32)

	# On linux, check gcc version.

	# OSX and Linux

	FIND_PATH(MAYA_INCLUDE_PATH maya/MTypes.h
		PATHS
			/usr/autodesk/maya/include
			$ENV{MAYA_LOCATION}/include
		DOC "The directory where MTypes.h resides")

# TODO

ENDIF (WIN32)



IF (MAYA_INCLUDE_PATH)
	SET( MAYA_FOUND 1 CACHE STRING "Set to 1 if Maya is found, 0 otherwise")
ELSE (MAYA_INCLUDE_PATH)
	SET( MAYA_FOUND 0 CACHE STRING "Set to 1 if Maya is found, 0 otherwise")
ENDIF (MAYA_INCLUDE_PATH)

MARK_AS_ADVANCED( MAYA_FOUND )
