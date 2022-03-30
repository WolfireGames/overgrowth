// This code is in the public domain -- Ignacio Castaño <castanyo@yahoo.es>

#ifndef NV_MATH_POLYGON_H
#define NV_MATH_POLYGON_H

#include <nvcore/Containers.h>

#include <nvmath/nvmath.h>
#include <nvmath/Vector.h>
#include <nvmath/Box.h>

namespace nv
{
	class Box;
	class Plane;
	class Triangle;
	

	class Polygon
	{
		NV_FORBID_COPY(Polygon);
	public:

		Polygon();
		Polygon(const Triangle & t);
		Polygon(const Vector3 * points, uint vertexCount);

		float area() const;
		Box bounds() const;
		Plane plane() const;

		uint clipTo(const Box & box);
		uint clipTo(const Plane & plane);

		void removeColinearPoints();
		
	private:

		Array<Vector3> pointArray;
	};


} // nv namespace

#endif	// NV_MATH_POLYGON_H
