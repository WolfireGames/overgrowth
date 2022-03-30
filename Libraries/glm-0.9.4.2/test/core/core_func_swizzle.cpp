///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-10-16
// Updated : 2011-10-16
// Licence : This source is under MIT License
// File    : test/core/core_func_swizzle.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GLM_FORCE_ONLY_XYZW
#	define GLM_FORCE_ONLY_XYZW
#endif
//#ifndef GLM_FORCE_PURE
//#	define GLM_FORCE_PURE
//#endif
#ifndef GLM_MESSAGES
#	define GLM_MESSAGES
#endif
#ifndef GLM_SWIZZLE
#	define GLM_SWIZZLE
#endif
#ifndef GLM_FORCE_CXX98
#	define GLM_FORCE_CXX98
#endif
#include <glm/glm.hpp>

int test_vec2_swizzle()
{
	int Error = 0;

    glm::ivec2 A(1, 2);
	glm::ivec2 B = A.xy();
	glm::ivec2 C(0);
	C.xy() = B.xy();

	Error += A == B ? 0 : 1;
	Error += A == C ? 0 : 1;

	return Error;
}

int test_vec3_swizzle()
{
	int Error = 0;

    glm::ivec3 A(1, 2, 3);
	glm::ivec3 B = A.xyz();
	glm::ivec3 C(0);
	C.xyz() = B.xyz();

	Error += A == B ? 0 : 1;
	Error += A == C ? 0 : 1;

	return Error;
}

int test_vec4_swizzle()
{
	int Error = 0;

    glm::ivec4 A(1, 2, 3, 4);
	glm::ivec4 B = A.xyzw();
	glm::ivec4 C(0);
	C.xyzw() = B.xyzw();

	Error += A == B ? 0 : 1;
	Error += A == C ? 0 : 1;

	return Error;
}

int main()
{
    int Error = 0;
    
	Error += test_vec2_swizzle();
	Error += test_vec3_swizzle();
    Error += test_vec4_swizzle();
        
	return Error;
}



