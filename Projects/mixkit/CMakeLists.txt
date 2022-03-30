CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)
PROJECT(mixkit)

IF(WIN32)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4244")
ENDIF()

SET(SRCDIR "${CMAKE_SOURCE_DIR}/../Libraries/qslim-2.1/mixkit/")
SET(LIBDIR "${CMAKE_SOURCE_DIR}/../Libraries")

INCLUDE_DIRECTORIES(
    ${SRCDIR}/src
)

FILE(GLOB MIXKIT_SRCS
    ${SRCDIR}/src/mixmops.cxx
    ${SRCDIR}/src/mixmsg.cxx
    ${SRCDIR}/src/MxAsp.cxx
    ${SRCDIR}/src/MxBlockModel.cxx
    ${SRCDIR}/src/MxCamera.cxx
    ${SRCDIR}/src/MxCmdParser.cxx
    ${SRCDIR}/src/MxDualModel.cxx
    ${SRCDIR}/src/MxDualSlim.cxx
    ${SRCDIR}/src/MxEdgeFilter.cxx
    ${SRCDIR}/src/MxFaceTree-2.cxx
    ${SRCDIR}/src/MxFaceTree.cxx
    ${SRCDIR}/src/MxFeatureFilter.cxx
    ${SRCDIR}/src/MxFeatureFilter.h
    ${SRCDIR}/src/MxFitFrame-2.cxx
    ${SRCDIR}/src/MxFitFrame.cxx
    ${SRCDIR}/src/MxFitFrame-vis.cxx
    ${SRCDIR}/src/MxFrame.cxx
    ${SRCDIR}/src/MxGeom2D.cxx
    ${SRCDIR}/src/MxGeom3D.cxx
    ${SRCDIR}/src/MxGLDebug.cxx
    ${SRCDIR}/src/MxGLUtils.cxx
    ${SRCDIR}/src/MxHeap.cxx
    ${SRCDIR}/src/MxMat3-jacobi.cxx
    ${SRCDIR}/src/MxMat4-jacobi.cxx
    ${SRCDIR}/src/MxMatrix.cxx
    ${SRCDIR}/src/MxPropSlim.cxx
    ${SRCDIR}/src/MxQMetric3.cxx
    ${SRCDIR}/src/MxQMetric.cxx
    ${SRCDIR}/src/MxQSlim.cxx
    ${SRCDIR}/src/MxQVis3.cxx
    ${SRCDIR}/src/MxSMF.cxx
    ${SRCDIR}/src/MxStdModel.cxx
    ${SRCDIR}/src/MxStdRender.cxx
    ${SRCDIR}/src/MxStdSlim.cxx
    ${SRCDIR}/src/MxTriProject.cxx
)

ADD_LIBRARY(mixkit STATIC ${MIXKIT_SRCS})

