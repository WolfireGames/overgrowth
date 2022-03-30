///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2008-08-31
// Licence : This source is under MIT License
// File    : test/core/type_mat3x4.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>

static bool test_operators()
{
	glm::mat3x4 l(1.0f);
	glm::mat3x4 m(1.0f);
	glm::vec3 u(1.0f);
	glm::vec4 v(1.0f);
	float x = 1.0f;
	glm::vec4 a = m * u;
	glm::vec3 b = v * m;
	glm::mat3x4 n = x / m;
	glm::mat3x4 o = m / x;
	glm::mat3x4 p = x * m;
	glm::mat3x4 q = m * x;
	bool R = m != q;
	bool S = m == l;

	return (S && !R) ? 0 : 1;
}

int main()
{
	int Error = 0;

	Error += test_operators();

	return Error;
}

