///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Mathematics (glm.g-truc.net)
///
/// Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///
/// @ref test
/// @file test/gtc/half_float.cpp
/// @date 2011-05-32 / 2012-04-07
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////


#include <glm/glm.hpp>
#include <glm/gtc/half_float.hpp>

int test_half_precision_scalar()
{
	int Error = 0;

	Error += sizeof(glm::half) == 2 ? 0 : 1;

	return Error;
}

int test_half_precision_vec()
{
	int Error = 0;

	Error += sizeof(glm::hvec2) == 4 ? 0 : 1;
	Error += sizeof(glm::hvec3) == 6 ? 0 : 1;
	Error += sizeof(glm::hvec4) == 8 ? 0 : 1;
    
    return Error;
}

int test_half_precision_mat()
{
	int Error = 0;

	Error += sizeof(glm::hmat2) == 8 ? 0 : 1;
	Error += sizeof(glm::hmat3) == 18 ? 0 : 1;
	Error += sizeof(glm::hmat4) == 32 ? 0 : 1;

	Error += sizeof(glm::hmat2x2) == 8 ? 0 : 1;
	Error += sizeof(glm::hmat2x3) == 12 ? 0 : 1;
	Error += sizeof(glm::hmat2x4) == 16 ? 0 : 1;
	Error += sizeof(glm::hmat3x2) == 12 ? 0 : 1;
	Error += sizeof(glm::hmat3x3) == 18 ? 0 : 1;
	Error += sizeof(glm::hmat3x4) == 24 ? 0 : 1;
	Error += sizeof(glm::hmat4x2) == 16 ? 0 : 1;
	Error += sizeof(glm::hmat4x3) == 24 ? 0 : 1;
	Error += sizeof(glm::hmat4x4) == 32 ? 0 : 1;

    return Error;
}

int test_half_ctor_mat2x2()
{
	int Error = 0;

	{
		glm::hvec2 A(1, 2);
		glm::hvec2 B(3, 4);
		glm::hmat2 C(A, B);//, 2.0f, 3.0f, 4.0f);
		glm::hmat2 D(1, 2, 3, 4);

		Error += C[0] == D[0] ? 0 : 1;
		Error += C[1] == D[1] ? 0 : 1;
	}

	{
		glm::hvec2 A(1, 2.0);
		glm::hvec2 B(3, 4.0);
		glm::hmat2 C(A, B);//, 2.0f, 3.0f, 4.0f);
		glm::hmat2 D(1, 2.0, 3u, 4.0f);

		Error += C[0] == D[0] ? 0 : 1;
		Error += C[1] == D[1] ? 0 : 1;
	}

	{
		glm::hmat2 A(1);
		glm::mat2 B(1);
		glm::hmat2 C(A);

		Error += A == C ? 0 : 1;
	}

    return Error;
}

int test_half_ctor_mat2x3()
{
	int Error = 0;
	
	{
		glm::hvec3 A(1, 2, 3);
		glm::hvec3 B(4, 5, 6);
		glm::hmat2x3 C(A, B);
		glm::hmat2x3 D(1, 2, 3, 4, 5, 6);
		
		Error += C[0] == D[0] ? 0 : 1;
		Error += C[1] == D[1] ? 0 : 1;
	}
	
	{
		glm::hvec3 A(1.0, 2.0f, 3u);
		glm::hvec3 B(4, 5u, 6u);
		glm::hmat2x3 C(A, B);
		glm::hmat2x3 D(1, 2.0, 3u, 4.0f, 5.0, 6);
		
		Error += C[0] == D[0] ? 0 : 1;
		Error += C[1] == D[1] ? 0 : 1;
	}
	
	{
		glm::hmat2x3 A(1);
		glm::mat2x3 B(1);
		glm::hmat2x3 C(A);
		
		Error += A == C ? 0 : 1;
	}
	
    return Error;
}

int test_half_ctor_mat2x4()
{
	int Error = 0;
	
	{
		glm::hvec4 A(1, 2, 3, 4);
		glm::hvec4 B(5, 6, 7, 8);
		glm::hmat2x4 C(A, B);
		glm::hmat2x4 D(1, 2, 3, 4, 5, 6, 7, 8);
		
		Error += C[0] == D[0] ? 0 : 1;
		Error += C[1] == D[1] ? 0 : 1;
	}
	
	{
		glm::hvec4 A(1.0, 2.0f, 3u, 4u);
		glm::hvec4 B(5u, 6u, 7.0, 8.0);
		glm::hmat2x4 C(A, B);
		glm::hmat2x4 D(1, 2.0, 3u, 4.0f, 5.0, 6, 7.0f, 8.0f);
		
		Error += C[0] == D[0] ? 0 : 1;
		Error += C[1] == D[1] ? 0 : 1;
	}
	
	{
		glm::hmat2x4 A(1);
		glm::mat2x4 B(1);
		glm::hmat2x4 C(A);
		
		Error += A == C ? 0 : 1;
	}
	
    return Error;
}

int test_half_ctor_mat3x2()
{
	int Error = 0;
	
	{
		glm::hvec2 A(1, 2);
		glm::hvec2 B(3, 4);
		glm::hvec2 C(5, 6);
		glm::hmat3x2 M(A, B, C);
		glm::hmat3x2 N(1, 2, 3, 4, 5, 6);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hvec2 A(1, 2.0);
		glm::hvec2 B(3, 4.0f);
		glm::hvec2 C(5u, 6.0f);
		glm::hmat3x2 M(A, B, C);
		glm::hmat3x2 N(1, 2.0, 3u, 4.0f, 5, 6);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hmat3x2 A(1);
		glm::mat3x2 B(1);
		glm::hmat3x2 C(A);
		
		Error += A == C ? 0 : 1;
	}
	
    return Error;
}

int test_half_ctor_mat3x3()
{
	int Error = 0;
	
	{
		glm::hvec3 A(1, 2, 3);
		glm::hvec3 B(4, 5, 6);
		glm::hvec3 C(7, 8, 9);
		glm::hmat3x3 M(A, B, C);
		glm::hmat3x3 N(1, 2, 3, 4, 5, 6, 7, 8, 9);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hvec3 A(1, 2.0, 3.0f);
		glm::hvec3 B(4, 5.0f, 6.0);
		glm::hvec3 C(7u, 8.0f, 9);
		glm::hmat3x3 M(A, B, C);
		glm::hmat3x3 N(1, 2.0, 3u, 4.0f, 5, 6, 7.0f, 8.0, 9u);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hmat3x3 A(1);
		glm::mat3x3 B(1);
		glm::hmat3x3 C(A);
		
		Error += A == C ? 0 : 1;
	}
	
    return Error;
}

int test_half_ctor_mat3x4()
{
	int Error = 0;
	
	{
		glm::hvec4 A(1, 2, 3, 4);
		glm::hvec4 B(5, 6, 7, 8);
		glm::hvec4 C(9, 10, 11, 12);
		glm::hmat3x4 M(A, B, C);
		glm::hmat3x4 N(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hvec4 A(1, 2.0, 3.0f, 4u);
		glm::hvec4 B(5, 6.0f, 7.0, 8);
		glm::hvec4 C(9u, 10.0f, 11, 12.f);
		glm::hmat3x4 M(A, B, C);
		glm::hmat3x4 N(1, 2.0, 3u, 4.0f, 5, 6, 7.0f, 8.0, 9u, 10, 11.f, 12.0);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hmat3x4 A(1);
		glm::mat3x4 B(1);
		glm::hmat3x4 C(A);
		
		Error += A == C ? 0 : 1;
	}
	
    return Error;
}

int test_half_ctor_mat4x2()
{
	int Error = 0;
	
	{
		glm::hvec2 A(1, 2);
		glm::hvec2 B(3, 4);
		glm::hvec2 C(5, 6);
		glm::hvec2 D(7, 8);
		glm::hmat4x2 M(A, B, C, D);
		glm::hmat4x2 N(1, 2, 3, 4, 5, 6, 7, 8);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hvec2 A(1, 2.0);
		glm::hvec2 B(3.0f, 4);
		glm::hvec2 C(5.0, 6u);
		glm::hvec2 D(7, 8u);
		glm::hmat4x2 M(A, B, C, D);
		glm::hmat4x2 N(1, 2.0, 3u, 4.0f, 5u, 6.0, 7, 8.0f);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hmat4x2 A(1);
		glm::mat4x2 B(1);
		glm::hmat4x2 C(A);
		
		Error += A == C ? 0 : 1;
	}
	
	return Error;
}

int test_half_ctor_mat4x3()
{
	int Error = 0;
	
	{
		glm::hvec3 A(1, 2, 3);
		glm::hvec3 B(4, 5, 6);
		glm::hvec3 C(7, 8, 9);
		glm::hvec3 D(10, 11, 12);
		glm::hmat4x3 M(A, B, C, D);
		glm::hmat4x3 N(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hvec3 A(1, 2.0, 3u);
		glm::hvec3 B(4.0f, 5, 6u);
		glm::hvec3 C(7.0, 8u, 9.f);
		glm::hvec3 D(10, 11u, 12.0);
		glm::hmat4x3 M(A, B, C, D);
		glm::hmat4x3 N(1, 2.0, 3u, 4.0f, 5u, 6.0, 7, 8.0f, 9, 10u, 11.f, 12.0);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hmat4x3 A(1);
		glm::mat4x3 B(1);
		glm::hmat4x3 C(A);
		
		Error += A == C ? 0 : 1;
	}
	
	return Error;
}

int test_half_ctor_mat4x4()
{
	int Error = 0;
	
	{
		glm::hvec4 A(1, 2, 3, 4);
		glm::hvec4 B(5, 6, 7, 8);
		glm::hvec4 C(9, 10, 11, 12);
		glm::hvec4 D(13, 14, 15, 16);
		glm::hmat4x4 M(A, B, C, D);
		glm::hmat4x4 N(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hvec4 A(1, 2.0, 3u, 4);
		glm::hvec4 B(5.0f, 6, 7u, 8.0);
		glm::hvec4 C(9.0, 10u, 11.f, 12);
		glm::hvec4 D(13, 14u, 15.0, 16u);
		glm::hmat4x4 M(A, B, C, D);
		glm::hmat4x4 N(1, 2.0, 3u, 4.0f, 5u, 6.0, 7, 8.0f, 9, 10u, 11.f, 12.0, 13, 14u, 15.0f, 16.0);
		
		Error += M == N ? 0 : 1;
	}
	
	{
		glm::hmat4x4 A(1);
		glm::mat4x4 B(1);
		glm::hmat4x4 C(A);
		
		Error += A == C ? 0 : 1;
	}
	
	return Error;
}

int test_half_ctor_vec2()
{
	int Error = 0;
	
	{
		glm::hvec2 A;
		A.x = glm::half(1);
		A.y = glm::half(2);
		//glm::hvec2 A(1, 2);
		glm::hvec2 B(A);
		glm::vec2 C(1, 2);
		glm::hvec2 D(C);
		glm::dvec2 E(1, 2);
		glm::hvec2 F(E);
		glm::hvec2 G(1, 2.0);
		glm::hvec2 H;
		H = A;
		
		Error += A == B ? 0 : 1;
		//Error += C == D ? 0 : 1; //Error
		//Error += E == F ? 0 : 1; //Error
		Error += A == G ? 0 : 1;
		Error += A == H ? 0 : 1;
	}
	
	{
		glm::hvec2 A(1);
		glm::vec2 B(1);
		glm::hvec2 C(A);

		Error += A == C ? 0 : 1;
	}
	
	return Error;
}

int test_half_ctor_vec3()
{
	int Error = 0;
	
	{
		glm::hvec3 A(1, 2, 3);
		glm::hvec3 B(A);
		glm::vec3 C(1, 2, 3);
		glm::hvec3 D(C);
		glm::dvec3 E(1, 2, 3);
		glm::hvec3 F(E);
		glm::hvec3 G(1, 2.0, 3);
		glm::hvec3 H;
		H = A;
		
		Error += A == B ? 0 : 1;
		//Error += C == D ? 0 : 1;
		//Error += E == F ? 0 : 1;
		Error += A == G ? 0 : 1;
		Error += A == H ? 0 : 1;
	}
	
	{
		glm::hvec3 A(1);
		glm::vec3 B(1);
		glm::hvec3 C(B);
		
		Error += A == C ? 0 : 1;
	}
	
	return Error;
}

int test_half_ctor_vec4()
{
	int Error = 0;
	
	{
		glm::hvec4 A(1, 2, 3, 4);
		glm::hvec4 B(A);
		glm::vec4 C(1, 2, 3, 4);
		glm::hvec4 D(C);
		glm::dvec4 E(1, 2, 3, 4);
		glm::hvec4 F(E);
		glm::hvec4 G(1, 2.0, 3, 4);
		glm::hvec4 H;
		H = A;
		
		Error += A == B ? 0 : 1;
		//Error += C == D ? 0 : 1;
		//Error += E == F ? 0 : 1;
		Error += A == G ? 0 : 1;
		Error += A == H ? 0 : 1;
	}
	
	{
		glm::hvec4 A(1);
		glm::vec4 B(1);
		glm::hvec4 C(B);
		
		Error += A == C ? 0 : 1;
	}
	
	return Error;
}

int test_hvec2_size()
{
	int Error = 0;
	
	Error += sizeof(glm::hvec2) <= sizeof(glm::lowp_vec2) ? 0 : 1;
	Error += 4 == sizeof(glm::hvec2) ? 0 : 1;
	Error += glm::hvec2().length() == 2 ? 0 : 1;
	
	return Error;
}

int test_hvec3_size()
{
	int Error = 0;
	
	Error += sizeof(glm::hvec3) <= sizeof(glm::lowp_vec3) ? 0 : 1;
	Error += 6 <= sizeof(glm::hvec3) ? 0 : 1;
	Error += glm::hvec3().length() == 3 ? 0 : 1;
		
	return Error;
}

int test_hvec4_size()
{
	int Error = 0;
	
	Error += sizeof(glm::hvec4) <= sizeof(glm::lowp_vec4) ? 0 : 1;
	Error += 8 <= sizeof(glm::hvec4) ? 0 : 1;
	Error += glm::hvec4().length() == 4 ? 0 : 1;
	
	return Error;
}

int main()
{
	int Error = 0;

	Error += test_hvec2_size();
	Error += test_hvec3_size();
	Error += test_hvec4_size();
	Error += test_half_ctor_vec2();
	Error += test_half_ctor_vec3();
	Error += test_half_ctor_vec4();
	Error += test_half_ctor_mat2x2();
	Error += test_half_ctor_mat2x3();
	Error += test_half_ctor_mat2x4();
	Error += test_half_ctor_mat3x2();
	Error += test_half_ctor_mat3x3();
	Error += test_half_ctor_mat3x4();
	Error += test_half_ctor_mat4x2();
	Error += test_half_ctor_mat4x3();
	Error += test_half_ctor_mat4x4();
	Error += test_half_precision_scalar();
	Error += test_half_precision_vec();
	Error += test_half_precision_mat();

	return Error;
}
