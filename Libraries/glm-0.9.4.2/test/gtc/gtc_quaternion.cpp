///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-16
// Updated : 2011-05-25
// Licence : This source is under MIT licence
// File    : test/gtc/quaternion.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/epsilon.hpp>

int test_quat_angle()
{
	int Error = 0;

	{
		glm::quat Q = glm::angleAxis(45.0f, glm::vec3(0, 0, 1));
		glm::quat N = glm::normalize(Q);
		float L = glm::length(N);
		Error += glm::epsilonEqual(L, 1.0f, 0.01f) ? 0 : 1;
		float A = glm::angle(N);
		Error += glm::epsilonEqual(A, 45.0f, 0.01f) ? 0 : 1;
	}
	{
		glm::quat Q = glm::angleAxis(45.0f, glm::normalize(glm::vec3(0, 1, 1)));
		glm::quat N = glm::normalize(Q);
		float L = glm::length(N);
		Error += glm::epsilonEqual(L, 1.0f, 0.01f) ? 0 : 1;
		float A = glm::angle(N);
		Error += glm::epsilonEqual(A, 45.0f, 0.01f) ? 0 : 1;
	}
	{
		glm::quat Q = glm::angleAxis(45.0f, glm::normalize(glm::vec3(1, 2, 3)));
		glm::quat N = glm::normalize(Q);
		float L = glm::length(N);
		Error += glm::epsilonEqual(L, 1.0f, 0.01f) ? 0 : 1;
		float A = glm::angle(N);
		Error += glm::epsilonEqual(A, 45.0f, 0.01f) ? 0 : 1;
	}

	return Error;
}

int test_quat_angleAxis()
{
	int Error = 0;

	glm::quat A = glm::angleAxis(0.0f, glm::vec3(0, 0, 1));
	glm::quat B = glm::angleAxis(90.0f, glm::vec3(0, 0, 1));
	glm::quat C = glm::mix(A, B, 0.5f);
	glm::quat D = glm::angleAxis(45.0f, glm::vec3(0, 0, 1));

	Error += glm::epsilonEqual(C.x, D.x, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.y, D.y, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.z, D.z, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.w, D.w, 0.01f) ? 0 : 1;

	return Error;
}

int test_quat_mix()
{
	int Error = 0;

	glm::quat A = glm::angleAxis(0.0f, glm::vec3(0, 0, 1));
	glm::quat B = glm::angleAxis(90.0f, glm::vec3(0, 0, 1));
	glm::quat C = glm::mix(A, B, 0.5f);
	glm::quat D = glm::angleAxis(45.0f, glm::vec3(0, 0, 1));

	Error += glm::epsilonEqual(C.x, D.x, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.y, D.y, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.z, D.z, 0.01f) ? 0 : 1;
	Error += glm::epsilonEqual(C.w, D.w, 0.01f) ? 0 : 1;

	return Error;
}

int test_quat_precision()
{
	int Error = 0;

	Error += sizeof(glm::lowp_quat) <= sizeof(glm::mediump_quat) ? 0 : 1;
	Error += sizeof(glm::mediump_quat) <= sizeof(glm::highp_quat) ? 0 : 1;

	return Error;
}

int test_quat_normalize()
{
	int Error(0);

	{
		glm::quat Q = glm::angleAxis(45.0f, glm::vec3(0, 0, 1));
		glm::quat N = glm::normalize(Q);
		float L = glm::length(N);
		Error += glm::epsilonEqual(L, 1.0f, 0.000001f) ? 0 : 1;
	}
	{
		glm::quat Q = glm::angleAxis(45.0f, glm::vec3(0, 0, 2));
		glm::quat N = glm::normalize(Q);
		float L = glm::length(N);
		Error += glm::epsilonEqual(L, 1.0f, 0.000001f) ? 0 : 1;
	}
	{
		glm::quat Q = glm::angleAxis(45.0f, glm::vec3(1, 2, 3));
		glm::quat N = glm::normalize(Q);
		float L = glm::length(N);
		Error += glm::epsilonEqual(L, 1.0f, 0.000001f) ? 0 : 1;
	}

	return Error;
}

int test_quat_euler()
{
	int Error(0);

	{
		glm::quat q(1.0f, 0.0f, 0.0f, 1.0f);
		float Roll = glm::roll(q);
		float Pitch = glm::pitch(q);
		float Yaw = glm::yaw(q);
		glm::vec3 Angles = glm::eulerAngles(q);
	}

	{
		glm::dquat q(1.0f, 0.0f, 0.0f, 1.0f);
		double Roll = glm::roll(q);
		double Pitch = glm::pitch(q);
		double Yaw = glm::yaw(q);
		glm::dvec3 Angles = glm::eulerAngles(q);
	}

	{
		glm::hquat q(glm::half(1.0f), glm::half(0.0f), glm::half(0.0f), glm::half(1.0f));
		glm::half Roll = glm::roll(q);
		glm::half Pitch = glm::pitch(q);
		glm::half Yaw = glm::yaw(q);
		glm::hvec3 Angles = glm::eulerAngles(q);
	}

	return Error;
}

int test_quat_slerp()
{
	int Error(0);

	float const Epsilon = 0.0001f;//glm::epsilon<float>();

	float sqrt2 = sqrt(2.0f)/2.0f;
	glm::quat id;
	glm::quat Y90rot(sqrt2, 0.0f, sqrt2, 0.0f);
	glm::quat Y180rot(0.0f, 0.0f, 1.0f, 0.0f);

	// Testing a == 0
	// Must be id
	glm::quat id2 = glm::slerp(id, Y90rot, 0.0f);
	Error += glm::all(glm::epsilonEqual(id, id2, Epsilon)) ? 0 : 1;

	// Testing a == 1
	// Must be 90° rotation on Y : 0 0.7 0 0.7
	glm::quat Y90rot2 = glm::slerp(id, Y90rot, 1.0f);
	Error += glm::all(glm::epsilonEqual(Y90rot, Y90rot2, Epsilon)) ? 0 : 1;

	// Testing standard, easy case
	// Must be 45° rotation on Y : 0 0.38 0 0.92
	glm::quat Y45rot1 = glm::slerp(id, Y90rot, 0.5f);

	// Testing reverse case
	// Must be 45° rotation on Y : 0 0.38 0 0.92
	glm::quat Ym45rot2 = glm::slerp(Y90rot, id, 0.5f);

	// Testing against full circle around the sphere instead of shortest path
	// Must be 45° rotation on Y
	// certainly not a 135° rotation
	glm::quat Y45rot3 = glm::slerp(id , -Y90rot, 0.5f);
	float Y45angle3 = glm::angle(Y45rot3);
	Error += glm::epsilonEqual(Y45angle3, 45.f, Epsilon) ? 0 : 1;
	Error += glm::all(glm::epsilonEqual(Ym45rot2, Y45rot3, Epsilon)) ? 0 : 1;

	// Same, but inverted
	// Must also be 45° rotation on Y :  0 0.38 0 0.92
	// -0 -0.38 -0 -0.92 is ok too
	glm::quat Y45rot4 = glm::slerp(-Y90rot, id, 0.5f);
	Error += glm::all(glm::epsilonEqual(Ym45rot2, -Y45rot4, Epsilon)) ? 0 : 1;

	// Testing q1 = q2
	// Must be 90° rotation on Y : 0 0.7 0 0.7
	glm::quat Y90rot3 = glm::slerp(Y90rot, Y90rot, 0.5f);
	Error += glm::all(glm::epsilonEqual(Y90rot, Y90rot3, Epsilon)) ? 0 : 1;

	// Testing 180° rotation
	// Must be 90° rotation on almost any axis that is on the XZ plane
	glm::quat XZ90rot = glm::slerp(id, -Y90rot, 0.5f);
	float XZ90angle = glm::angle(XZ90rot); // Must be PI/4 = 0.78;
	Error += glm::epsilonEqual(XZ90angle, 45.f, Epsilon) ? 0 : 1;

	// Testing almost equal quaternions (this test should pass through the linear interpolation)
	// Must be 0 0.00X 0 0.99999
	glm::quat almostid = glm::slerp(id, glm::angleAxis(0.1f, 0.0f, 1.0f, 0.0f), 0.5f);

	return Error;
}

int test_quat_type()
{
	glm::quat A;
	glm::dquat B;

	return 0;
}

int main()
{
	int Error(0);

	Error += test_quat_precision();
	Error += test_quat_type();
	Error += test_quat_angle();
	Error += test_quat_angleAxis();
	Error += test_quat_mix();
	Error += test_quat_normalize();
	Error += test_quat_euler();
	Error += test_quat_slerp();

	return Error;
}
