// This code is in the public domain -- castanyo@yahoo.es

#include <nvmath/Basis.h>

using namespace nv;


/// Normalize basis vectors.
void Basis::normalize(float epsilon /*= NV_EPSILON*/)
{
	normal = ::normalize(normal, epsilon);
	tangent = ::normalize(tangent, epsilon);
	bitangent = ::normalize(bitangent, epsilon);
}


/// Gram-Schmidt orthogonalization.
/// @note Works only if the vectors are close to orthogonal.
void Basis::orthonormalize(float epsilon /*= NV_EPSILON*/)
{
	// N' = |N|
	// T' = |T - (N' dot T) N'|
	// B' = |B - (N' dot B) N' - (T' dot B) T'|

	normal = ::normalize(normal, epsilon);

	tangent -= normal * dot(normal, tangent);
	tangent = ::normalize(tangent, epsilon);

	bitangent -= normal * dot(normal, bitangent);
	bitangent -= tangent * dot(tangent, bitangent);
	bitangent = ::normalize(bitangent, epsilon);
}


/// Robust orthonormalization. 
/// Returns an orthonormal basis even when the original is degenerate.
void Basis::robustOrthonormalize(float epsilon /*= NV_EPSILON*/)
{
	if (length(normal) < epsilon)
	{
		normal = cross(tangent, bitangent);
		
		if (length(normal) < epsilon)
		{
			tangent = Vector3(1, 0, 0);
			bitangent = Vector3(0, 1, 0);
			normal = Vector3(0, 0, 1);
			return;
		}
	}
	normal = nv::normalize(normal, epsilon);
	
	tangent -= normal * dot(normal, tangent);
	bitangent -= normal * dot(normal, bitangent);
	
	if (length(tangent) < epsilon)
	{
		if (length(bitangent) < epsilon)
		{
			buildFrameForDirection(normal);
		}
		else
		{
			tangent = cross(bitangent, normal);
			nvCheck(isNormalized(tangent, epsilon));
		}
	}
	else
	{
#if 0
		tangent = nv::normalize(tangent, epsilon);
		bitangent -= tangent * dot(tangent, bitangent);
		
		if (length(bitangent) < epsilon)
		{
			bitangent = cross(tangent, normal);
			nvCheck(isNormalized(bitangent));
		}
		else
		{
			bitangent = nv::normalize(bitangent, epsilon);
		}
#else
		if (length(bitangent) < epsilon)
		{
			bitangent = cross(tangent, normal);
			nvCheck(isNormalized(bitangent));
		}
		else
		{
			tangent = nv::normalize(tangent);
			bitangent = nv::normalize(bitangent);
			
			Vector3 bisector;
			if (length(tangent + bitangent) < epsilon) 
			{
				bisector = tangent;
			}
			else 
			{
				bisector = nv::normalize(tangent + bitangent);
			}
			Vector3 axis = cross(bisector, normal);
			
			nvDebugCheck(isNormalized(axis, epsilon));
			nvDebugCheck(equal(dot(axis, tangent), -dot(axis, bitangent), epsilon));
			
			if (dot(axis, tangent) > 0)
			{
				tangent = nv::normalize(bisector + axis);
				bitangent = nv::normalize(bisector - axis);
			}
			else
			{
				tangent = nv::normalize(bisector - axis);
				bitangent = nv::normalize(bisector + axis);
			}
		}
#endif
	}
	
	/*// Check vector lengths.
	if (!isNormalized(normal, epsilon))
	{
		nvDebug("%f %f %f\n", normal.x(), normal.y(), normal.z());
		nvDebug("%f %f %f\n", tangent.x(), tangent.y(), tangent.z());
		nvDebug("%f %f %f\n", bitangent.x(), bitangent.y(), bitangent.z());
	}*/
	
	nvCheck(isNormalized(normal, epsilon));
	nvCheck(isNormalized(tangent, epsilon));
	nvCheck(isNormalized(bitangent, epsilon));

	// Check vector angles.
	nvCheck(equal(dot(normal, tangent), 0.0f, epsilon));
	nvCheck(equal(dot(normal, bitangent), 0.0f, epsilon));
	nvCheck(equal(dot(tangent, bitangent), 0.0f, epsilon));

	// Check vector orientation.
	const float det = dot(cross(normal, tangent), bitangent);
	nvCheck(equal(det, 1.0f, epsilon) || equal(det, -1.0f, epsilon));
}


/// Build an arbitrary frame for the given direction.
void Basis::buildFrameForDirection(Vector3::Arg d)
{
	nvCheck(isNormalized(d));
	normal = d;

	// Choose minimum axis.
	if (fabsf(normal.x()) < fabsf(normal.y()) && fabsf(normal.x()) < fabsf(normal.z()))
	{
		tangent = Vector3(1, 0, 0);
	}
	else if (fabsf(normal.y()) < fabsf(normal.z()))
	{
		tangent = Vector3(0, 1, 0);
	}
	else
	{
		tangent = Vector3(0, 0, 1);
	}

	// Ortogonalize
	tangent -= normal * dot(normal, tangent);
	tangent = ::normalize(tangent);

	bitangent = cross(normal, tangent);
}

bool Basis::isValid() const
{
	if (equal(normal, Vector3(zero))) return false;
	if (equal(tangent, Vector3(zero))) return false;
	if (equal(bitangent, Vector3(zero))) return false;

	if (equal(determinant(), 0.0f)) return false;
	
	return true;
}


/// Transform by this basis. (From this basis to object space).
Vector3 Basis::transform(Vector3::Arg v) const
{
	Vector3 o = tangent * v.x();
	o += bitangent * v.y();
	o += normal * v.z();
	return o;
}

/// Transform by the transpose. (From object space to this basis).
Vector3 Basis::transformT(Vector3::Arg v)
{
	return Vector3(dot(tangent, v), dot(bitangent, v), dot(normal, v));
}

/// Transform by the inverse. (From object space to this basis).
/// @note Uses Cramer's rule so the inverse is not accurate if the basis is ill-conditioned.
Vector3 Basis::transformI(Vector3::Arg v) const
{
	const float det = determinant();
	nvDebugCheck(!equal(det, 0.0f, 0.0f));
	
	const float idet = 1.0f / det;

	// Rows of the inverse matrix.
	Vector3 r0(
		 (bitangent.y() * normal.z() - bitangent.z() * normal.y()),
		-(bitangent.x() * normal.z() - bitangent.z() * normal.x()),
		 (bitangent.x() * normal.y() - bitangent.y() * normal.x()));

	Vector3 r1(
		-(tangent.y() * normal.z() - tangent.z() * normal.y()),
		 (tangent.x() * normal.z() - tangent.z() * normal.x()),
		-(tangent.x() * normal.y() - tangent.y() * normal.x()));

	Vector3 r2(
		 (tangent.y() * bitangent.z() - tangent.z() * bitangent.y()),
		-(tangent.x() * bitangent.z() - tangent.z() * bitangent.x()),
		 (tangent.x() * bitangent.y() - tangent.y() * bitangent.x()));

	return Vector3(dot(v, r0), dot(v, r1), dot(v, r2)) * idet;
}	


