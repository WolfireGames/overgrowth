///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2008-08-31
// Licence : This source is under MIT License
// File    : test/core/type_vec1.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLM_SWIZZLE
#include <glm/glm.hpp>

static int test_operators()
{
	glm::vec4 A(1.0f);
	glm::vec4 B(1.0f);
	bool R = A != B;
	bool S = A == B;

	return (S && !R) ? 0 : 1;
}

int main()
{
	int Error = 0;

	Error += test_operators();
	
	return Error;
}
