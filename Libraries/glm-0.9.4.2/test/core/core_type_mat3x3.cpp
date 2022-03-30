///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2008-08-31
// Licence : This source is under MIT License
// File    : test/core/type_mat3x3.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <cstdio>

void print(glm::dmat3 const & Mat0)
{
	printf("mat3(\n");
	printf("\tvec3(%2.3f, %2.3f, %2.3f)\n", Mat0[0][0], Mat0[0][1], Mat0[0][2]);
	printf("\tvec3(%2.3f, %2.3f, %2.3f)\n", Mat0[1][0], Mat0[1][1], Mat0[1][2]);
	printf("\tvec3(%2.3f, %2.3f, %2.3f))\n\n", Mat0[2][0], Mat0[2][1], Mat0[2][2]);
}

int test_mat3x3()
{
	glm::dmat3 Mat0(
		glm::dvec3(0.6f, 0.2f, 0.3f), 
		glm::dvec3(0.2f, 0.7f, 0.5f), 
		glm::dvec3(0.3f, 0.5f, 0.7f));
	glm::dmat3 Inv0 = glm::inverse(Mat0);
	glm::dmat3 Res0 = Mat0 * Inv0;

	print(Mat0);
	print(Inv0);
	print(Res0);

	return 0;
}

static int test_operators()
{
	glm::mat3x3 l(1.0f);
	glm::mat3x3 m(1.0f);
	glm::vec3 u(1.0f);
	glm::vec3 v(1.0f);
	float x = 1.0f;
	glm::vec3 a = m * u;
	glm::vec3 b = v * m;
	glm::mat3x3 n = x / m;
	glm::mat3x3 o = m / x;
	glm::mat3x3 p = x * m;
	glm::mat3x3 q = m * x;
	bool R = m != q;
	bool S = m == l;

	return (S && !R) ? 0 : 1;
}

int test_inverse()
{
	int Error(0);

	{
		glm::mat3 const Matrix(
			glm::vec3(0.6f, 0.2f, 0.3f), 
			glm::vec3(0.2f, 0.7f, 0.5f), 
			glm::vec3(0.3f, 0.5f, 0.7f));
		glm::mat3 const Inverse = glm::inverse(Matrix);
		glm::mat3 const Identity = Matrix * Inverse;

		Error += glm::all(glm::epsilonEqual(Identity[0], glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[1], glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[2], glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.01f))) ? 0 : 1;
	}

	{
		glm::mat3 const Matrix(
			glm::vec3(0.6f, 0.2f, 0.3f), 
			glm::vec3(0.2f, 0.7f, 0.5f), 
			glm::vec3(0.3f, 0.5f, 0.7f));
		glm::mat3 const Identity = Matrix / Matrix;

		Error += glm::all(glm::epsilonEqual(Identity[0], glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[1], glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.01f))) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(Identity[2], glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.01f))) ? 0 : 1;
	}

	return Error;
}

int main()
{
	int Error = 0;

	Error += test_mat3x3();
	Error += test_operators();
	Error += test_inverse();

	return Error;
}

