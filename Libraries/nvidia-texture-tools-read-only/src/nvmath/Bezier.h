// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_BEZIER_H
#define NV_MATH_BEZIER_H

#include <nvmath/nvmath.h>
#include <nvmath/Vector.h>

namespace nv
{

	void evaluateCubicBezierPatch(float u, float v, 
		Vector3::Arg p0, Vector3::Arg p1, Vector3::Arg p2, Vector3::Arg p3,
		Vector3::Arg p4, Vector3::Arg p5, Vector3::Arg p6, Vector3::Arg p7,
		Vector3::Arg p8, Vector3::Arg p9, Vector3::Arg pA, Vector3::Arg pB,
		Vector3::Arg pC, Vector3::Arg pD, Vector3::Arg pE, Vector3::Arg pF, 
		Vector3 * pos, Vector3 * du, Vector3 * dv);

	void degreeElevate(Vector3::Arg p0, Vector3::Arg p1, Vector3::Arg p2, Vector3::Arg p3,
		Vector3 * q0, Vector3 * q1, Vector3 * q2, Vector3 * q3, Vector3 * q4);

	void evaluateQuarticBezierTriangle(float u, float v, 
		Vector3::Arg p0, Vector3::Arg p1, Vector3::Arg p2, Vector3::Arg p3, Vector3::Arg p4,
		Vector3::Arg p5, Vector3::Arg p6, Vector3::Arg p7, Vector3::Arg p8,
		Vector3::Arg p9, Vector3::Arg pA, Vector3::Arg pB, 
		Vector3::Arg pC, Vector3::Arg pD, 
		Vector3::Arg pE, 
		Vector3 * pos, Vector3 * du, Vector3 * dv);


} // nv namespace

#endif // NV_MATH_BEZIER_H
