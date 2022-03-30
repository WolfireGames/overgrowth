PROJECT(nvmath)

SET(MATH_SRCS
	nvmath.h
	Vector.h
	Matrix.h
	Quaternion.h
	Plane.h Plane.cpp
	Box.h
	Color.h
	Frustum.h Frustum.cpp
	Montecarlo.h Montecarlo.cpp
	Random.h Random.cpp
	SphericalHarmonic.h SphericalHarmonic.cpp
	Basis.h Basis.cpp
	Triangle.h Triangle.cpp TriBox.cpp
	Polygon.h Polygon.cpp
	TypeSerialization.h TypeSerialization.cpp
	Sparse.h Sparse.cpp
	Solver.h Solver.cpp
	KahanSum.h
	Half.h Half.cpp
	Fitting.h Fitting.cpp
	Bezier.h Bezier.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# targets
ADD_DEFINITIONS(-DNVMATH_EXPORTS)

IF(NVMATH_SHARED)
	ADD_LIBRARY(nvmath SHARED ${MATH_SRCS})
ELSE(NVMATH_SHARED)
	ADD_LIBRARY(nvmath ${MATH_SRCS})
ENDIF(NVMATH_SHARED)

TARGET_LINK_LIBRARIES(nvmath ${LIBS} nvcore)

INSTALL(TARGETS nvmath
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)
