///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-16
// Updated : 2010-09-16
// Licence : This source is under MIT licence
// File    : test/gtc/swizzle.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>
#include <glm/gtc/swizzle.hpp>

int test_swizzle_vec4_ref_dynamic()
{
	int Error = 0;

	{
		glm::ivec4 A(0, 1, 2, 3);
		glm::ivec4 B(2, 1, 0, 3);
		glm::swizzle(A, glm::Z, glm::Y, glm::X, glm::W) = B;
		Error += (A.z == B.x && A.y == B.y && A.x == B.z && A.w == B.w) ? 0 : 1;
	}

	{
		glm::ivec4 A(0, 1, 2, 3);
		glm::ivec3 B(2, 1, 0);
		glm::swizzle(A, glm::Z, glm::Y, glm::X) = B;
		Error += (A.z == B.x && A.y == B.y && A.x == B.z) ? 0 : 1;
	}

	{
		glm::ivec4 A(0, 1, 2, 3);
		glm::ivec2 B(2, 1);
		glm::swizzle(A, glm::Z, glm::Y) = B;
		Error += (A.z == B.x && A.y == B.y) ? 0 : 1;
	}

	{
		glm::ivec4 A(0, 1, 2, 3);
		int B(2);
		glm::swizzle(A, glm::Z) = B;
		Error += (A.z == B) ? 0 : 1;
	}

	return Error;
}

int test_swizzle_vec4_ref_static()
{
	int Error = 0;

	{
		glm::ivec4 A(0, 1, 2, 3);
		glm::ivec4 B(2, 1, 0, 3);
		glm::swizzle<glm::Z, glm::Y, glm::X, glm::W>(A) = B;
		Error += (A.z == B.x && A.y == B.y && A.x == B.z && A.w == B.w) ? 0 : 1;
	}

	{
		glm::ivec4 A(0, 1, 2, 3);
		glm::ivec3 B(2, 1, 0);
		glm::swizzle<glm::Z, glm::Y, glm::X>(A) = B;
		Error += (A.z == B.x && A.y == B.y && A.x == B.z) ? 0 : 1;
	}

	{
		glm::ivec4 A(0, 1, 2, 3);
		glm::ivec2 B(2, 1);
		glm::swizzle<glm::Z, glm::Y>(A) = B;
		Error += (A.z == B.x && A.y == B.y) ? 0 : 1;
	}

	{
		glm::ivec4 A(0, 1, 2, 3);
		int B(2);
		glm::swizzle<glm::Z>(A) = B;
		Error += (A.z == B) ? 0 : 1;
	}

	return Error;
}

int test_swizzle_vec4_const_dynamic()
{
	int Error = 0;

	glm::ivec4 A(0, 1, 2, 3);
	glm::ivec4 B = glm::swizzle(A, glm::B, glm::G, glm::R, glm::A);
	Error += (glm::all(glm::equal(glm::ivec4(2, 1, 0, 3), B))) ? 0 : 1;

	glm::ivec3 C = glm::swizzle(A, glm::W, glm::Y, glm::Z);
	Error += (glm::all(glm::equal(glm::ivec3(3, 1, 2), C))) ? 0 : 1;

	glm::ivec2 D = glm::swizzle(A, glm::W, glm::X);
	Error += (glm::all(glm::equal(glm::ivec2(3, 0), D))) ? 0 : 1;

	int E = glm::swizzle(A, glm::Q);
	Error += (E == 3) ? 0 : 1;

	return Error;
}

int test_swizzle_vec4_const_static()
{
	int Error = 0;

	glm::ivec4 A(0, 1, 2, 3);

	glm::ivec4 B = glm::swizzle<glm::B, glm::G, glm::R, glm::A>(A);
	Error += (glm::all(glm::equal(glm::ivec4(2, 1, 0, 3), B))) ? 0 : 1;

	glm::ivec3 C = glm::swizzle<glm::W, glm::Y, glm::Z>(A);
	Error += (glm::all(glm::equal(glm::ivec3(3, 1, 2), C))) ? 0 : 1;

	glm::ivec2 D = glm::swizzle<glm::W, glm::X>(A);
	Error += (glm::all(glm::equal(glm::ivec2(3, 0), D))) ? 0 : 1;

	int E = glm::swizzle<glm::Q>(A);
	Error += (E == 3) ? 0 : 1;

	return Error;
}

int test_swizzle_vec3_partial()
{
	int Error = 0;

	glm::ivec3 A(0, 1, 2);

	{
		glm::ivec3 B(A.swizzle(glm::R, glm::G, glm::B));	
		Error += (A == B) ? 0 : 1;
	}

	{
		glm::ivec3 B(A.swizzle(glm::R, glm::G), 2);	
		Error += (A == B) ? 0 : 1;
	}

	{
		glm::ivec3 B(0, A.swizzle(glm::G, glm::B));	
		Error += (A == B) ? 0 : 1;
	}

	return Error;
}

int test_swizzle_vec4_partial()
{
	int Error = 0;

	glm::ivec4 A(0, 1, 2, 3);

	{
		glm::ivec4 B(A.swizzle(glm::R, glm::G, glm::B), 3);	
		Error += (A == B) ? 0 : 1;
	}

	{
		glm::ivec4 B(A.swizzle(glm::R, glm::G), 2, 3);	
		Error += (A == B) ? 0 : 1;
	}

	{
		glm::ivec4 B(0, A.swizzle(glm::G, glm::B), 3);	
		Error += (A == B) ? 0 : 1;
	}

	{
		glm::ivec4 B(0, 1, A.swizzle(glm::B, glm::A));	
		Error += (A == B) ? 0 : 1;
	}

	{
		glm::ivec4 B(A.swizzle(glm::X, glm::Y), A.swizzle(glm::Z, glm::W));	
		Error += (A == B) ? 0 : 1;
	}

	{
		glm::ivec4 B(A.swizzle(glm::X, glm::Y), glm::vec2(2, 3));	
		Error += (A == B) ? 0 : 1;
	}

	{
		glm::ivec4 B(glm::vec2(0, 1), A.swizzle(glm::Z, glm::W));	
		Error += (A == B) ? 0 : 1;
	}

	return Error;
}

int main()
{
	int Error = 0;
	Error += test_swizzle_vec3_partial();
	Error += test_swizzle_vec4_ref_dynamic();
	Error += test_swizzle_vec4_ref_static();
	Error += test_swizzle_vec4_const_dynamic();
	Error += test_swizzle_vec4_const_static();
	Error += test_swizzle_vec4_partial();

	return Error;
}
