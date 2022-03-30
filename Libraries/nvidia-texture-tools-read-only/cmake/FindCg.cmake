#
# Try to find NVIDIA's Cg compiler, runtime libraries, and include path.
# Once done this will define
#
# CG_FOUND =system has NVIDIA Cg and it can be used. 
# CG_INCLUDE_PATH = directory where cg.h resides
# CG_LIBRARY = full path to libCg.so (Cg.DLL on win32)
# CG_GL_LIBRARY = full path to libCgGL.so (CgGL.dll on win32)
# CG_COMPILER = full path to cgc (cgc.exe on win32)
# 

# On OSX default to using the framework version of Cg.
IF (APPLE)
  INCLUDE(${CMAKE_ROOT}/Modules/CMakeFindFrameworks.cmake)
  SET(CG_FRAMEWORK_INCLUDES)
  CMAKE_FIND_FRAMEWORKS(Cg)
  IF (Cg_FRAMEWORKS)
    FOREACH(dir ${Cg_FRAMEWORKS})
      SET(CG_FRAMEWORK_INCLUDES ${CG_FRAMEWORK_INCLUDES}
        ${dir}/Headers ${dir}/PrivateHeaders)
    ENDFOREACH(dir)

    # Find the include  dir
    FIND_PATH(CG_INCLUDE_PATH cg.h
      ${CG_FRAMEWORK_INCLUDES}
      )

    # Since we are using Cg framework, we must link to it.
	# Note, we use weak linking, so that it works even when Cg is not available.
    SET(CG_LIBRARY "-weak_framework Cg" CACHE STRING "Cg library")
    SET(CG_GL_LIBRARY "-weak_framework Cg" CACHE STRING "Cg GL library")
  ENDIF (Cg_FRAMEWORKS)
  FIND_PROGRAM(CG_COMPILER cgc
    /usr/bin
    /usr/local/bin
    DOC "The Cg compiler"
    )
ELSE (APPLE)
  IF (WIN32)
 
    # When compiling 64-bit programs, the binaries and libs are in bin.x64 and lib.x64 directories,
	
	# This will have only effect for 64bit versions of cmake, when running the default 32bit version
	# both ProgramFiles and ProgramFiles(x86) point to the same place in Win64
	SET(PFx86_VARNAME "ProgramFiles(x86)")
    SET(PFx86 $ENV{${PFx86_VARNAME}})
	
	# Let's play safe in case we are cross compiling to 64 bit: for cgc it doesn't really matter
	FIND_PROGRAM( CG_COMPILER cgc
      $ENV{CG_BIN64_PATH}
	  $ENV{CG_BIN_PATH}
      $ENV{PROGRAMFILES}/NVIDIA\ Corporation/Cg/bin
	  $ENV{PFx86}/NVIDIA\ Corporation/Cg/bin
      $ENV{PROGRAMFILES}/Cg
      ${PROJECT_SOURCE_DIR}/../Cg
      DOC "The Cg Compiler"
    )

    IF (CG_COMPILER)
      GET_FILENAME_COMPONENT(CG_COMPILER_DIR ${CG_COMPILER} PATH)
      GET_FILENAME_COMPONENT(CG_COMPILER_SUPER_DIR ${CG_COMPILER_DIR} PATH)
    ELSE (CG_COMPILER)
      SET (CG_COMPILER_DIR .)
      SET (CG_COMPILER_SUPER_DIR ..)
    ENDIF (CG_COMPILER)
    FIND_PATH( CG_INCLUDE_PATH Cg/cg.h
      $ENV{CG_INC_PATH}
      $ENV{PROGRAMFILES}/NVIDIA\ Corporation/Cg/include
      $ENV{PROGRAMFILES}/Cg
      ${PROJECT_SOURCE_DIR}/../Cg
      ${CG_COMPILER_SUPER_DIR}/include
      ${CG_COMPILER_DIR}
      DOC "The directory where Cg/cg.h resides"
      )
	
	IF (NV_SYSTEM_PROCESSOR STREQUAL "AMD64")
      FIND_LIBRARY( CG_LIBRARY
        NAMES Cg
        PATHS
        $ENV{CG_LIB64_PATH}
        $ENV{PROGRAMFILES}/NVIDIA\ Corporation/Cg/lib.x64
		$ENV{PFx86}/NVIDIA\ Corporation/Cg/lib.x64
        $ENV{PROGRAMFILES}/Cg
        $ENV{PFx86}/Cg
        ${PROJECT_SOURCE_DIR}/../Cg
        ${CG_COMPILER_SUPER_DIR}/lib.x64
        ${CG_COMPILER_DIR}
        DOC "The Cg runtime library (64-bit)"
        )
      FIND_LIBRARY( CG_GL_LIBRARY
        NAMES CgGL
        PATHS
		$ENV{CG_LIB64_PATH}
        $ENV{PROGRAMFILES}/NVIDIA\ Corporation/Cg/lib.x64
		$ENV{PFx86}/NVIDIA\ Corporation/Cg/lib.x64
        $ENV{PROGRAMFILES}/Cg
		$ENV{PFx86}/Cg
        ${PROJECT_SOURCE_DIR}/../Cg
        ${CG_COMPILER_SUPER_DIR}/lib.x64
        ${CG_COMPILER_DIR}
        DOC "The Cg GL runtime library (64-bit)"
        )
	ELSE(NV_SYSTEM_PROCESSOR STREQUAL "AMD64")
      FIND_LIBRARY( CG_LIBRARY
        NAMES Cg
        PATHS
        $ENV{CG_LIB_PATH}
        $ENV{PROGRAMFILES}/NVIDIA\ Corporation/Cg/lib
        $ENV{PROGRAMFILES}/Cg
        ${PROJECT_SOURCE_DIR}/../Cg
        ${CG_COMPILER_SUPER_DIR}/lib
        ${CG_COMPILER_DIR}
        DOC "The Cg runtime library"
        )
      FIND_LIBRARY( CG_GL_LIBRARY
        NAMES CgGL
        PATHS
		$ENV{CG_LIB_PATH}
        $ENV{PROGRAMFILES}/NVIDIA\ Corporation/Cg/lib
        $ENV{PROGRAMFILES}/Cg
        ${PROJECT_SOURCE_DIR}/../Cg
        ${CG_COMPILER_SUPER_DIR}/lib
        ${CG_COMPILER_DIR}
        DOC "The Cg GL runtime library"
        )
	ENDIF(NV_SYSTEM_PROCESSOR STREQUAL "AMD64")

  ELSE (WIN32)
    FIND_PROGRAM( CG_COMPILER cgc
      /usr/bin
      /usr/local/bin
      DOC "The Cg Compiler"
      )
    GET_FILENAME_COMPONENT(CG_COMPILER_DIR "${CG_COMPILER}" PATH)
    GET_FILENAME_COMPONENT(CG_COMPILER_SUPER_DIR "${CG_COMPILER_DIR}" PATH)
    FIND_PATH( CG_INCLUDE_PATH Cg/cg.h
      /usr/include
      /usr/local/include
      ${CG_COMPILER_SUPER_DIR}/include
      DOC "The directory where Cg/cg.h resides"
      )
    FIND_LIBRARY( CG_LIBRARY Cg
      PATHS
      /usr/lib64
      /usr/lib
      /usr/local/lib64
      /usr/local/lib
      ${CG_COMPILER_SUPER_DIR}/lib64
      ${CG_COMPILER_SUPER_DIR}/lib
      DOC "The Cg runtime library"
      )
	SET(CG_LIBRARY ${CG_LIBRARY} -lpthread)
    FIND_LIBRARY( CG_GL_LIBRARY CgGL
      PATHS
      /usr/lib64
      /usr/lib
      /usr/local/lib64
      /usr/local/lib
      ${CG_COMPILER_SUPER_DIR}/lib64
      ${CG_COMPILER_SUPER_DIR}/lib
      DOC "The Cg runtime library"
      )
  ENDIF (WIN32)
ENDIF (APPLE)

IF (CG_INCLUDE_PATH)
  SET( CG_FOUND 1 CACHE STRING "Set to 1 if CG is found, 0 otherwise")
ELSE (CG_INCLUDE_PATH)
  SET( CG_FOUND 0 CACHE STRING "Set to 1 if CG is found, 0 otherwise")
ENDIF (CG_INCLUDE_PATH)

MARK_AS_ADVANCED( CG_FOUND )
