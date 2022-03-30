// This code is in the public domain -- castanyo@yahoo.es

#include "Bezier.h"


using namespace nv;


static void deCasteljau(float u, Vector3::Arg p0, Vector3::Arg p1, Vector3::Arg p2, Vector3::Arg p3, Vector3 * p)
{
	Vector3 q0 = lerp(p0, p1, u);
	Vector3 q1 = lerp(p1, p2, u);
	Vector3 q2 = lerp(p2, p3, u);
	Vector3 r0 = lerp(q0, q1, u);
	Vector3 r1 = lerp(q1, q2, u);

	*p = lerp(r0, r1, u);
}

static void deCasteljau(float u, Vector3::Arg p0, Vector3::Arg p1, Vector3::Arg p2, Vector3::Arg p3, Vector3 * p, Vector3 * dp)
{
	Vector3 q0 = lerp(p0, p1, u);
	Vector3 q1 = lerp(p1, p2, u);
	Vector3 q2 = lerp(p2, p3, u);
	Vector3 r0 = lerp(q0, q1, u);
	Vector3 r1 = lerp(q1, q2, u);
	
	*dp = r0 - r1;
	*p = lerp(r0, r1, u);
}


void nv::evaluateCubicBezierPatch(float u, float v, 
	Vector3::Arg p0, Vector3::Arg p1, Vector3::Arg p2, Vector3::Arg p3,
	Vector3::Arg p4, Vector3::Arg p5, Vector3::Arg p6, Vector3::Arg p7,
	Vector3::Arg p8, Vector3::Arg p9, Vector3::Arg pA, Vector3::Arg pB,
	Vector3::Arg pC, Vector3::Arg pD, Vector3::Arg pE, Vector3::Arg pF, 
	Vector3 * pos, Vector3 * du, Vector3 * dv)
{
#if 0
	Vector2 L0(1-u,1-v);
	Vector2 L1(u,v);

	Vector2 Q0 =     L0 * L0;
	Vector2 Q1 = 2 * L0 * L1;
	Vector2 Q2 =     L1 * L1;

	Vector2 B0 =     L0 * L0 * L0;
	Vector2 B1 = 3 * L0 * L0 * L1;
	Vector2 B2 = 3 * L1 * L1 * L0;
	Vector2 B3 =     L1 * L1 * L1;

	*pos = 
		(B0.x() * p0 + B1.x() * p1 + B2.x() * p2 + B3.x() * p3) * B0.y() +
		(B0.x() * p4 + B1.x() * p5 + B2.x() * p6 + B3.x() * p7) * B1.y() +
		(B0.x() * p8 + B1.x() * p9 + B2.x() * pA + B3.x() * pB) * B2.y() +
		(B0.x() * pC + B1.x() * pD + B2.x() * pE + B3.x() * pF) * B3.y();

	*du =
		((p0-p1) * B0.y() + (p4-p5) * B1.y() + (p8-p9) * B2.y() + (pC-pD) * B3.y()) * Q0.x() +
		((p1-p2) * B0.y() + (p5-p6) * B1.y() + (p9-pA) * B2.y() + (pD-pE) * B3.y()) * Q1.x() +
		((p2-p3) * B0.y() + (p6-p7) * B1.y() + (pA-pB) * B2.y() + (pE-pF) * B3.y()) * Q2.x();

	*dv =
		((p0-p4) * B0.x() + (p1-p5) * B1.x() + (p2-p6) * B2.x() + (p3-p7) * B3.x()) * Q0.y() +
		((p4-p8) * B0.x() + (p5-p9) * B1.x() + (p6-pA) * B2.x() + (p7-pB) * B3.x()) * Q1.y() +
		((p8-pC) * B0.x() + (p9-pD) * B1.x() + (pA-pE) * B2.x() + (pB-pF) * B3.x()) * Q2.y();
#else
	Vector3 t0, t1, t2, t3;
	Vector3 q0, q1, q2, q3;

	deCasteljau(u, p0, p1, p2, p3, &q0, &t0);
	deCasteljau(u, p4, p5, p6, p7, &q1, &t1);
	deCasteljau(u, p8, p9, pA, pB, &q2, &t2);
	deCasteljau(u, pC, pD, pE, pF, &q3, &t3);

	deCasteljau(v, q0, q1, q2, q3, pos, dv);
	deCasteljau(v, t0, t1, t2, t3, du);
#endif
}


void nv::degreeElevate(Vector3::Arg p0, Vector3::Arg p1, Vector3::Arg p2, Vector3::Arg p3,
	Vector3 * q0, Vector3 * q1, Vector3 * q2, Vector3 * q3, Vector3 * q4)
{
}

void nv::evaluateQuarticBezierTriangle(float u, float v, 
	Vector3::Arg p0, Vector3::Arg p1, Vector3::Arg p2, Vector3::Arg p3, Vector3::Arg p4,
	Vector3::Arg p5, Vector3::Arg p6, Vector3::Arg p7, Vector3::Arg p8,
	Vector3::Arg p9, Vector3::Arg pA, Vector3::Arg pB, 
	Vector3::Arg pC, Vector3::Arg pD, 
	Vector3::Arg pE, 
	Vector3 * pos, Vector3 * du, Vector3 * dv)
{
}

