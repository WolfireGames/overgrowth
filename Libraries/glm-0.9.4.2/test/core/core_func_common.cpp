///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-01-15
// Updated : 2011-09-13
// Licence : This source is under MIT licence
// File    : test/core/func_common.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

//#include <boost/array.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/thread/thread.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <cstdio>

int test_modf()
{
	int Error(0);

	{
		float X(1.5f);
		float I(0.0f);
		float A = glm::modf(X, I);

		Error += I == 1.0f ? 0 : 1;
		Error += A == 0.5f ? 0 : 1;
	}

	{
		glm::vec4 X(1.1f, 1.2f, 1.5f, 1.7f);
		glm::vec4 I(0.0f);
		glm::vec4 A = glm::modf(X, I);

		Error += I == glm::vec4(1.0f) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(A, glm::vec4(0.1f, 0.2f, 0.5f, 0.7f), 0.00001f)) ? 0 : 1;
	}

	{
		glm::dvec4 X(1.1, 1.2, 1.5, 1.7);
		glm::dvec4 I(0.0);
		glm::dvec4 A = glm::modf(X, I);

		Error += I == glm::dvec4(1.0) ? 0 : 1;
		Error += glm::all(glm::epsilonEqual(A, glm::dvec4(0.1, 0.2, 0.5, 0.7), 0.000000001)) ? 0 : 1;
	}

	{
		double X(1.5);
		double I(0.0);
		double A = glm::modf(X, I);

		Error += I == 1.0 ? 0 : 1;
		Error += A == 0.5 ? 0 : 1;
	}

	return Error;
}

int test_floatBitsToInt()
{
	int Error = 0;
	
	{
		float A = 1.0f;
		int B = glm::floatBitsToInt(A);
		float C = glm::intBitsToFloat(B);
		int D = *(int*)&A;
		Error += B == D ? 0 : 1;
		Error += A == C ? 0 : 1;
	}

	{
		glm::vec2 A(1.0f, 2.0f);
		glm::ivec2 B = glm::floatBitsToInt(A);
		glm::vec2 C = glm::intBitsToFloat(B);
		Error += B.x == *(int*)&(A.x) ? 0 : 1;
		Error += B.y == *(int*)&(A.y) ? 0 : 1;
		Error += A == C? 0 : 1;
	}

	{
		glm::vec3 A(1.0f, 2.0f, 3.0f);
		glm::ivec3 B = glm::floatBitsToInt(A);
		glm::vec3 C = glm::intBitsToFloat(B);
		Error += B.x == *(int*)&(A.x) ? 0 : 1;
		Error += B.y == *(int*)&(A.y) ? 0 : 1;
		Error += B.z == *(int*)&(A.z) ? 0 : 1;
		Error += A == C? 0 : 1;
	}
	
	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::ivec4 B = glm::floatBitsToInt(A);
		glm::vec4 C = glm::intBitsToFloat(B);
		Error += B.x == *(int*)&(A.x) ? 0 : 1;
		Error += B.y == *(int*)&(A.y) ? 0 : 1;
		Error += B.z == *(int*)&(A.z) ? 0 : 1;
		Error += B.w == *(int*)&(A.w) ? 0 : 1;
		Error += A == C? 0 : 1;
	}
	
	return Error;
}

int test_floatBitsToUint()
{
	int Error = 0;
	
	{
		float A = 1.0f;
		glm::uint B = glm::floatBitsToUint(A);
		float C = glm::intBitsToFloat(B);
		Error += B == *(glm::uint*)&A ? 0 : 1;
		Error += A == C? 0 : 1;
	}
	
	{
		glm::vec2 A(1.0f, 2.0f);
		glm::uvec2 B = glm::floatBitsToUint(A);
		glm::vec2 C = glm::uintBitsToFloat(B);
		Error += B.x == *(glm::uint*)&(A.x) ? 0 : 1;
		Error += B.y == *(glm::uint*)&(A.y) ? 0 : 1;
		Error += A == C ? 0 : 1;
	}
	
	{
		glm::vec3 A(1.0f, 2.0f, 3.0f);
		glm::uvec3 B = glm::floatBitsToUint(A);
		glm::vec3 C = glm::uintBitsToFloat(B);
		Error += B.x == *(glm::uint*)&(A.x) ? 0 : 1;
		Error += B.y == *(glm::uint*)&(A.y) ? 0 : 1;
		Error += B.z == *(glm::uint*)&(A.z) ? 0 : 1;
		Error += A == C? 0 : 1;
	}
	
	{
		glm::vec4 A(1.0f, 2.0f, 3.0f, 4.0f);
		glm::uvec4 B = glm::floatBitsToUint(A);
		glm::vec4 C = glm::uintBitsToFloat(B);
		Error += B.x == *(glm::uint*)&(A.x) ? 0 : 1;
		Error += B.y == *(glm::uint*)&(A.y) ? 0 : 1;
		Error += B.z == *(glm::uint*)&(A.z) ? 0 : 1;
		Error += B.w == *(glm::uint*)&(A.w) ? 0 : 1;
		Error += A == C? 0 : 1;
	}
	
	return Error;
}

int test_mix()
{
	int Error = 0;

	{
		float A = glm::mix(0.f, 1.f, true);
		Error += A == 1.f ? 0 : 1;
		float B = glm::mix(0.f, 1.f, false);
		Error += B == 0.f ? 0 : 1;
	}

	{
		float A = glm::mix(0.f, 1.f, 1.f);
		Error += A == 1.f ? 0 : 1;
		float B = glm::mix(0.f, 1.f, 0.f);
		Error += B == 0.f ? 0 : 1;
	}

	return Error;
}

int test_round()
{
	int Error = 0;
	
	{
		float A = glm::round(0.0f);
		Error += A == 0.0f ? 0 : 1;
		float B = glm::round(0.5f);
		Error += B == 1.0f ? 0 : 1;
		float C = glm::round(1.0f);
		Error += C == 1.0f ? 0 : 1;
		float D = glm::round(0.1f);
		Error += D == 0.0f ? 0 : 1;
		float E = glm::round(0.9f);
		Error += E == 1.0f ? 0 : 1;
		float F = glm::round(1.5f);
		Error += F == 2.0f ? 0 : 1;
		float G = glm::round(1.9f);
		Error += G == 2.0f ? 0 : 1;
	}
	
	{
		float A = glm::round(-0.0f);
		Error += A ==  0.0f ? 0 : 1;
		float B = glm::round(-0.5f);
		Error += B == -1.0f ? 0 : 1;
		float C = glm::round(-1.0f);
		Error += C == -1.0f ? 0 : 1;
		float D = glm::round(-0.1f);
		Error += D ==  0.0f ? 0 : 1;
		float E = glm::round(-0.9f);
		Error += E == -1.0f ? 0 : 1;
		float F = glm::round(-1.5f);
		Error += F == -2.0f ? 0 : 1;
		float G = glm::round(-1.9f);
		Error += G == -2.0f ? 0 : 1;
	}
	
	return Error;
}

int test_roundEven()
{
	int Error = 0;

	{
		float A = glm::roundEven(-1.5f);
		Error += glm::epsilonEqual(A, -2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(1.5f);
		Error += glm::epsilonEqual(A, 2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(-3.5f);
		Error += glm::epsilonEqual(A, -4.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(3.5f);
		Error += glm::epsilonEqual(A, 4.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(-2.5f);
		Error += glm::epsilonEqual(A, -2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(2.5f);
		Error += glm::epsilonEqual(A, 2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(-2.4f);
		Error += glm::epsilonEqual(A, -2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(2.4f);
		Error += glm::epsilonEqual(A, 2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(-2.6f);
		Error += glm::epsilonEqual(A, -3.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(2.6f);
		Error += glm::epsilonEqual(A, 3.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(-2.0f);
		Error += glm::epsilonEqual(A, -2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}
	{
		float A = glm::roundEven(2.0f);
		Error += glm::epsilonEqual(A, 2.0f, 0.0001f) ? 0 : 1;
		Error += 0;
	}

	{
		float A = glm::roundEven(0.0f);
		Error += A == 0.0f ? 0 : 1;
		float B = glm::roundEven(0.5f);
		Error += B == 0.0f ? 0 : 1;
		float C = glm::roundEven(1.0f);
		Error += C == 1.0f ? 0 : 1;
		float D = glm::roundEven(0.1f);
		Error += D == 0.0f ? 0 : 1;
		float E = glm::roundEven(0.9f);
		Error += E == 1.0f ? 0 : 1;
		float F = glm::roundEven(1.5f);
		Error += F == 2.0f ? 0 : 1;
		float G = glm::roundEven(1.9f);
		Error += G == 2.0f ? 0 : 1;
	}

	{
		float A = glm::roundEven(-0.0f);
		Error += A ==  0.0f ? 0 : 1;
		float B = glm::roundEven(-0.5f);
		Error += B == -0.0f ? 0 : 1;
		float C = glm::roundEven(-1.0f);
		Error += C == -1.0f ? 0 : 1;
		float D = glm::roundEven(-0.1f);
		Error += D ==  0.0f ? 0 : 1;
		float E = glm::roundEven(-0.9f);
		Error += E == -1.0f ? 0 : 1;
		float F = glm::roundEven(-1.5f);
		Error += F == -2.0f ? 0 : 1;
		float G = glm::roundEven(-1.9f);
		Error += G == -2.0f ? 0 : 1;
	}

	{
		float A = glm::roundEven(1.5f);
		Error += A == 2.0f ? 0 : 1;
		float B = glm::roundEven(2.5f);
		Error += B == 2.0f ? 0 : 1;
		float C = glm::roundEven(3.5f);
		Error += C == 4.0f ? 0 : 1;
		float D = glm::roundEven(4.5f);
		Error += D == 4.0f ? 0 : 1;
		float E = glm::roundEven(5.5f);
		Error += E == 6.0f ? 0 : 1;
		float F = glm::roundEven(6.5f);
		Error += F == 6.0f ? 0 : 1;
		float G = glm::roundEven(7.5f);
		Error += G == 8.0f ? 0 : 1;
	}
	
	{
		float A = glm::roundEven(-1.5f);
		Error += A == -2.0f ? 0 : 1;
		float B = glm::roundEven(-2.5f);
		Error += B == -2.0f ? 0 : 1;
		float C = glm::roundEven(-3.5f);
		Error += C == -4.0f ? 0 : 1;
		float D = glm::roundEven(-4.5f);
		Error += D == -4.0f ? 0 : 1;
		float E = glm::roundEven(-5.5f);
		Error += E == -6.0f ? 0 : 1;
		float F = glm::roundEven(-6.5f);
		Error += F == -6.0f ? 0 : 1;
		float G = glm::roundEven(-7.5f);
		Error += G == -8.0f ? 0 : 1;
	}

	return Error;
}

int test_isnan()
{
	int Error = 0;

	float Zero_f = 0.0;
	double Zero_d = 0.0;

	{
 		Error += true == glm::isnan(0.0/Zero_d) ? 0 : 1;
 		Error += true == glm::any(glm::isnan(glm::dvec2(0.0 / Zero_d))) ? 0 : 1;
 		Error += true == glm::any(glm::isnan(glm::dvec3(0.0 / Zero_d))) ? 0 : 1;
 		Error += true == glm::any(glm::isnan(glm::dvec4(0.0 / Zero_d))) ? 0 : 1;
	}

	{
 		Error += true == glm::isnan(0.0f/Zero_f) ? 0 : 1;
 		Error += true == glm::any(glm::isnan(glm::vec2(0.0f/Zero_f))) ? 0 : 1;
 		Error += true == glm::any(glm::isnan(glm::vec3(0.0f/Zero_f))) ? 0 : 1;
 		Error += true == glm::any(glm::isnan(glm::vec4(0.0f/Zero_f))) ? 0 : 1;
	}

	return Error;
}
 
int test_isinf()
{
	int Error = 0;
 
	float Zero_f = 0.0;
	double Zero_d = 0.0;

	{
 		Error += true == glm::isinf( 1.0/Zero_d) ? 0 : 1;
		Error += true == glm::isinf(-1.0/Zero_d) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::dvec2( 1.0/Zero_d))) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::dvec2(-1.0/Zero_d))) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::dvec3( 1.0/Zero_d))) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::dvec3(-1.0/Zero_d))) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::dvec4( 1.0/Zero_d))) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::dvec4(-1.0/Zero_d))) ? 0 : 1;
	}
 
	{
 		Error += true == glm::isinf( 1.0f/Zero_f) ? 0 : 1;
 		Error += true == glm::isinf(-1.0f/Zero_f) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::vec2( 1.0f/Zero_f))) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::vec2(-1.0f/Zero_f))) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::vec3( 1.0f/Zero_f))) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::vec3(-1.0f/Zero_f))) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::vec4( 1.0f/Zero_f))) ? 0 : 1;
 		Error += true == glm::any(glm::isinf(glm::vec4(-1.0f/Zero_f))) ? 0 : 1;
	}
 
	return Error;
}

int main()
{
	int Error(0);

	Error += test_modf();
	Error += test_floatBitsToInt();
	Error += test_floatBitsToUint();
	Error += test_mix();
	Error += test_round();
	Error += test_roundEven();
	Error += test_isnan();
	//Error += test_isinf();

	return Error;
}

