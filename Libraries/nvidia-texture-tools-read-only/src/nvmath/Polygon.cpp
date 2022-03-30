// This code is in the public domain -- Ignacio Castaño <castanyo@yahoo.es>

#include <nvmath/Polygon.h>

#include <nvmath/Triangle.h>
#include <nvmath/Plane.h>

using namespace nv;


Polygon::Polygon()
{
}

Polygon::Polygon(const Triangle & t)
{
	pointArray.resize(3);
	pointArray[0] = t.v[0];
	pointArray[1] = t.v[1];
	pointArray[2] = t.v[2];
}

Polygon::Polygon(const Vector3 * points, uint vertexCount)
{
	pointArray.resize(vertexCount);
	
	for (uint i = 0; i < vertexCount; i++)
	{
		pointArray[i] = points[i];
	}
}


/// Compute polygon area.
float Polygon::area() const
{
	float total = 0;
	
	const uint pointCount = pointArray.count();
    for (uint i = 2; i < pointCount; i++)
    {
		Vector3 v1 = pointArray[i-1] - pointArray[0];
		Vector3 v2 = pointArray[i] - pointArray[0];
		
		total += 0.5f * length(cross(v1, v2));
    }
	
    return total;	
}

/// Get the bounds of the polygon.
Box Polygon::bounds() const
{
	Box bounds;
	bounds.clearBounds();
	foreach(p, pointArray)
	{
		bounds.addPointToBounds(pointArray[p]);
	}
	return bounds;
}


/// Get the plane of the polygon.
Plane Polygon::plane() const
{
	// @@ Do something better than this?
	Vector3 n = cross(pointArray[1] - pointArray[0], pointArray[2] - pointArray[0]);
	return Vector4(n, dot(n, pointArray[0]));
}


/// Clip polygon to box.
uint Polygon::clipTo(const Box & box)
{
	const Plane posX( 1, 0, 0, box.maxCorner().x());
	const Plane negX(-1, 0, 0,-box.minCorner().x());
	const Plane posY( 0, 1, 0, box.maxCorner().y());
	const Plane negY( 0,-1, 0,-box.minCorner().y());
	const Plane posZ( 0, 0, 1, box.maxCorner().z());
	const Plane negZ( 0, 0,-1,-box.minCorner().z());

	if (clipTo(posX) == 0) return 0;
	if (clipTo(negX) == 0) return 0;
	if (clipTo(posY) == 0) return 0;
	if (clipTo(negY) == 0) return 0;
	if (clipTo(posZ) == 0) return 0;
	if (clipTo(negZ) == 0) return 0;
	
	return pointArray.count();
}


/// Clip polygon to plane.
uint Polygon::clipTo(const Plane & plane)
{
	int count = 0;

	const uint pointCount = pointArray.count();
	
	Array<Vector3> newPointArray(pointCount + 1);	// @@ Do not create copy every time.
	
	Vector3 prevPoint = pointArray[pointCount - 1];
	float prevDist = dot(plane.vector(), prevPoint) - plane.offset();
	
	for (uint i = 0; i < pointCount; i++)
	{
		const Vector3 point = pointArray[i];
		float dist = dot(plane.vector(), point) - plane.offset();
		
		// @@ Handle points on plane better.
		
		if (dist <= 0) // interior.
		{
			if (prevDist > 0) // exterior
			{
				// Add segment intersection point.
				Vector3 dp = point - prevPoint;
				
				float t = dist / prevDist;
				newPointArray.append(point - dp * t);
			}
			
			// Add interior point.
			newPointArray.append(point);
		}
		else if (dist > 0 && prevDist < 0)
		{
			// Add segment intersection point.
			Vector3 dp = point - prevPoint;
			
			float t = dist / prevDist;
			newPointArray.append(point - dp * t);
		}
		
		prevPoint = point;
		prevDist = dist;
	}

	swap(pointArray, newPointArray);

	return count;
}


void Polygon::removeColinearPoints()
{
	const uint pointCount = pointArray.count();
	
	Array<Vector3> newPointArray(pointCount);
	
    for (uint i = 0 ; i < pointCount; i++)
    {
        int j = (i + 1) % pointCount;
        int k = (i + pointCount - 1) % pointCount;
		
		Vector3 v1 = normalize(pointArray[j] - pointArray[i]);
		Vector3 v2 = normalize(pointArray[i] - pointArray[k]);
		
        if (dot(v1, v2) < 0.999)
        {
			newPointArray.append(pointArray[i]);
        }
    }

	swap(pointArray, newPointArray);
}

