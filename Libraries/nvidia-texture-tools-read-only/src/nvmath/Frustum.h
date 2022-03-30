// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_FRUSTUM_H
#define NV_MATH_FRUSTUM_H

#include <nvmath/Plane.h>

namespace nv
{
	class Box;

	class NVMATH_CLASS Frustum
	{
	public:
		// Construct a frustum from typical view parameters.  fovy has the same meaning as for glut_perspective_reshaper.
		// The resulting frustum is centred at the origin, pointing along the z-axis.
		Frustum()	{}
		Frustum(float fovy, float aspect, float near, float far);

		// Unlike some intersection methods, we don't bother to distinguish intersects
		// from contains.  A true result could indicate either.
		bool intersects(const Box&) const;

	private:
		friend Frustum transformFrustum(const Matrix&, const Frustum&);

		enum PlaneIndices { NEAR, LEFT, RIGHT, TOP, BOTTOM, FAR, PLANE_COUNT };
		Plane planes[PLANE_COUNT];
	};

	Frustum transformFrustum(const Matrix&, const Frustum&);
}

#endif
