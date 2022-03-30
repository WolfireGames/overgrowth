// This code is in the public domain -- castanyo@yahoo.es

#include "Frustum.h"
#include "Box.h"
#include "Matrix.h"

namespace nv
{
	Frustum::Frustum(float fovy, float aspect, float near, float far)
	{
		// Define a frustum looking along the -ve z axis.
		planes[NEAR] = Plane(Vector3(0,0,1), near);
		planes[FAR]  = Plane(Vector3(0,0,-1), far);

		const Vector3 origin(zero);
		const float tanfy = tan(fovy/2.0f);
		planes[TOP]    = Plane(Vector3( 1.0f, 0.0f, tanfy * aspect), origin);
		planes[BOTTOM] = Plane(Vector3(-1.0f, 0.0f, tanfy * aspect), origin);

		planes[RIGHT] = Plane(Vector3(0.0f,  1.0f, tanfy), origin);
		planes[LEFT]  = Plane(Vector3(0.0f, -1.0f, tanfy), origin);

		for(int p = 0; p < Frustum::PLANE_COUNT; ++p) 
			planes[p] = normalize(planes[p]);
	}

	static void getAllCorners(const Box& boxy, Vector3 c[])
	{
		c[0] = boxy.minCorner();
		c[1] = boxy.maxCorner();
		c[2] = Vector3(c[0].x(), c[1].y(), c[0].z());
		c[3] = Vector3(c[0].x(), c[1].y(), c[1].z());
		c[4] = Vector3(c[0].x(), c[0].y(), c[1].z());
		c[5] = Vector3(c[1].x(), c[1].y(), c[0].z());
		c[6] = Vector3(c[1].x(), c[0].y(), c[0].z());
		c[7] = Vector3(c[1].x(), c[0].y(), c[1].z());
	}

	bool Frustum::intersects(const Box& boxy) const
	{
		const int N_CORNERS = 8;
		Vector3 boxCorners[N_CORNERS];
		getAllCorners(boxy, boxCorners);	

		// test all 8 corners against the 6 sides
		// if all points are behind 1 specific plane, we are out
		// if we are in with all points, then we are fully in
		for(int p = 0; p < Frustum::PLANE_COUNT; ++p) 
		{
			int iInCount = N_CORNERS;
			
			for(int i = 0; i < N_CORNERS; ++i) 
			{			
				// test point [i] against the planes
				if (distance(planes[p], boxCorners[i]) > 0.0f)
					--iInCount;
			}		
			
			// were all the points outside of plane p?
			if (iInCount == 0)
				return false;		
		}	
		
		return true;
	}

	Frustum transformFrustum(const Matrix& m, const Frustum& f)
	{
		Frustum result;

		for (int i=0; i!=Frustum::PLANE_COUNT; ++i)
			result.planes[i] = transformPlane(m, f.planes[i]);

		return result;
	}
}
