///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-31
// Updated : 2011-09-20
// Licence : This source is under MIT licence
// File    : test/core/type_half.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>
#include <glm/gtc/half_float.hpp>

int test_half_ctor()
{
	int Error = 0;

	glm::half A(1.0f);
	Error += float(A) == 1.0f ? 0 : 1;

	glm::half B = glm::half(1.0f);
	Error += float(B) == 1.0f ? 0 : 1;

	glm::half C = B;
	Error += float(C) == 1.0f ? 0 : 1;

	return Error;
}

int test_half_cast()
{
	int Error = 0;

	glm::half A(2.0f);
	Error += float(A) == 2.0f ? 0 : 1;

	glm::half B(2.0);
	Error += float(B) == 2.0f ? 0 : 1;

	glm::half C(2);
	Error += float(C) == 2.0f ? 0 : 1;

	float D(A);
	Error += D == 2.0f ? 0 : 1;

	double E(A);
	Error += E == 2.0 ? 0 : 1;

	int F(A);
	Error += F == 2.0 ? 0 : 1;

	return Error;
}

int test_half_relational()
{
	int Error = 0;

	glm::half A(1.0f);
	glm::half B(2.0f);

	Error += !(A == B) ? 0 : 1;
	Error +=  (A != B) ? 0 : 1;
	Error +=  (A <= B) ? 0 : 1;
	Error +=  (A  < B) ? 0 : 1;
	Error += !(A >= B) ? 0 : 1;
	Error += !(A  > B) ? 0 : 1;

	return Error;
}

int test_half_arithmetic_unary_ops()
{
	int Error = 0;

	{
		glm::half A(2.0f);
		glm::half B(3.0f);
		A += B;
		Error += (A == glm::half( 5.0f) ? 0 : 1);
	}

	{
		glm::half A(2.0f);
		glm::half B(3.0f);
		A -= B;
		Error += (A == glm::half(-1.0f) ? 0 : 1);
	}

	{
		glm::half A(2.0f);
		glm::half B(3.0f);
		B -= A;
		Error += (B == glm::half( 1.0f) ? 0 : 1);
	}

	{
		glm::half A(2.0f);
		glm::half B(3.0f);
		A *= B;
		Error += (A == glm::half(6.0f) ? 0 : 1);
	}

	{
		glm::half A(2.0f);
		glm::half B(3.0f);
		A /= B;
		Error += (A == glm::half(2.0f / 3.0f) ? 0 : 1);
	}

	{
		glm::half A(2.0f);
		glm::half B(3.0f);
		B /= A;
		Error += (B == glm::half(3.0f / 2.0f) ? 0 : 1);
	}

	return Error;
}

int test_half_arithmetic_binary_ops()
{
	int Error = 0;

	glm::half A(2.0f);
	glm::half B(3.0f);

	Error += A + B == glm::half( 5.0f) ? 0 : 1;
	Error += A - B == glm::half(-1.0f) ? 0 : 1;
	Error += B - A == glm::half( 1.0f) ? 0 : 1;
	Error += A * B == glm::half( 6.0f) ? 0 : 1;
	Error += A / B == glm::half(2.0f / 3.0f) ? 0 : 1;
	Error += B / A == glm::half(3.0f / 2.0f) ? 0 : 1;

	return Error;
}

int test_half_arithmetic_counter_ops()
{
	int Error = 0;

	{
		glm::half A(2.0f);
		Error += A == glm::half(2.0f) ? 0 : 1;
	}

	{
		glm::half A(2.0f);
		glm::half B = A++;
		Error += B == glm::half(3.0f) ? 0 : 1;
	}

	{
		glm::half A(2.0f);
		glm::half B = A--;
		Error += B == glm::half(1.0f) ? 0 : 1;
	}

	{
		glm::half A(2.0f);
		glm::half B = ++A;
		Error += B == glm::half(3.0f) ? 0 : 1;
	}

	{
		glm::half A(2.0f);
		glm::half B = --A;
		Error += B == glm::half(1.0f) ? 0 : 1;
	}

	{
		glm::half A(2.0f);
		glm::half B = -A;
		Error += B == glm::half(-2.0f) ? 0 : 1;
	}

	return Error;
}

int main()
{
	int Result = 0;

	Result += test_half_ctor();
	Result += test_half_cast();
	Result += test_half_relational();
	Result += test_half_arithmetic_unary_ops();
	Result += test_half_arithmetic_binary_ops();
	Result += test_half_arithmetic_counter_ops();
	
	return Result;
}
