// This code is in the public domain -- icastano@gmail.com

#ifndef NV_MATH_FITTING_H
#define NV_MATH_FITTING_H

#include <nvmath/nvmath.h>
#include <nvmath/Vector.h>

namespace nv
{

	Vector3 ComputeCentroid(int n, const Vector3 * points, const float * weights, Vector3::Arg metric);
	void ComputeCovariance(int n, const Vector3 * points, const float * weights, Vector3::Arg metric, float * covariance);
	Vector3 ComputePrincipalComponent(int n, const Vector3 * points, const float * weights, Vector3::Arg metric);

	// Returns number of clusters [1-4].
	int Compute4Means(int n, const Vector3 * points, const float * weights, Vector3::Arg metric, Vector3 * cluster);

} // nv namespace

#endif // NV_MATH_FITTING_H
