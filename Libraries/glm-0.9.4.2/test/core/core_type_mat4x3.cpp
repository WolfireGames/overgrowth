///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2008-08-31
// Licence : This source is under MIT License
// File    : test/core/type_mat4x3.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>

static int test_operators()
{
	glm::mat4x3 l(1.0f);
	glm::mat4x3 m(1.0f);
	glm::vec4 u(1.0f);
	glm::vec3 v(1.0f);
	float x = 1.0f;
	glm::vec3 a = m * u;
	glm::vec4 b = v * m;
	glm::mat4x3 n = x / m;
	glm::mat4x3 o = m / x;
	glm::mat4x3 p = x * m;
	glm::mat4x3 q = m * x;
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


