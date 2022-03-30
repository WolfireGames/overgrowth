///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-01-15
// Updated : 2012-05-02
// Licence : This source is under MIT licence
// File    : test/core/func_matrix.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>

int test_matrixCompMult()
{
    int Error(0);
    
    {
        glm::mat2 m(0, 1, 2, 3);
        glm::mat2 n = glm::matrixCompMult(m, m);
        Error += n == glm::mat2(0, 1, 4, 9) ? 0 : 1;
    }

    {
        glm::mat2x3 m(0, 1, 2, 3, 4, 5);
        glm::mat2x3 n = glm::matrixCompMult(m, m);
        Error += n == glm::mat2x3(0, 1, 4, 9, 16, 25) ? 0 : 1;
    }
  
    {
        glm::mat2x4 m(0, 1, 2, 3, 4, 5, 6, 7);
        glm::mat2x4 n = glm::matrixCompMult(m, m);
        Error += n == glm::mat2x4(0, 1, 4, 9, 16, 25, 36, 49) ? 0 : 1;
    }
    
    {
        glm::mat3 m(0, 1, 2, 3, 4, 5, 6, 7, 8);
        glm::mat3 n = glm::matrixCompMult(m, m);
        Error += n == glm::mat3(0, 1, 4, 9, 16, 25, 36, 49, 64) ? 0 : 1;
    }
    
    {
        glm::mat3x2 m(0, 1, 2, 3, 4, 5);
        glm::mat3x2 n = glm::matrixCompMult(m, m);
        Error += n == glm::mat3x2(0, 1, 4, 9, 16, 25) ? 0 : 1;
    }
 
    {
        glm::mat3x4 m(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
        glm::mat3x4 n = glm::matrixCompMult(m, m);
        Error += n == glm::mat3x4(0, 1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121) ? 0 : 1;
    }
    
    {
        glm::mat4 m(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        glm::mat4 n = glm::matrixCompMult(m, m);
        Error += n == glm::mat4(0, 1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121, 144, 169, 196, 225) ? 0 : 1;
    }
    
    {
        glm::mat4x2 m(0, 1, 2, 3, 4, 5, 6, 7);
        glm::mat4x2 n = glm::matrixCompMult(m, m);
        Error += n == glm::mat4x2(0, 1, 4, 9, 16, 25, 36, 49) ? 0 : 1;
    }
    
    {
        glm::mat4x3 m(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
        glm::mat4x3 n = glm::matrixCompMult(m, m);
        Error += n == glm::mat4x3(0, 1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121) ? 0 : 1;
    }
    
	return Error;
}

int test_outerProduct()
{


	return 0;
}

int test_transpose()
{
    int Error(0);
    
    {
        glm::mat2 m(0, 1, 2, 3);
        glm::mat2 t = glm::transpose(m);
        Error += t == glm::mat2(0, 2, 1, 3) ? 0 : 1;
    }

    {
        glm::mat2x3 m(0, 1, 2, 3, 4, 5);
        glm::mat3x2 t = glm::transpose(m);
        Error += t == glm::mat3x2(0, 3, 1, 4, 2, 5) ? 0 : 1;
    }

    {
        glm::mat2x4 m(0, 1, 2, 3, 4, 5, 6, 7);
        glm::mat4x2 t = glm::transpose(m);
        Error += t == glm::mat4x2(0, 4, 1, 5, 2, 6, 3, 7) ? 0 : 1;
    }
    
    {
        glm::mat3 m(0, 1, 2, 3, 4, 5, 6, 7, 8);
        glm::mat3 t = glm::transpose(m);
        Error += t == glm::mat3(0, 3, 6, 1, 4, 7, 2, 5, 8) ? 0 : 1;
    }
    
    {
        glm::mat3x2 m(0, 1, 2, 3, 4, 5);
        glm::mat2x3 t = glm::transpose(m);
        Error += t == glm::mat2x3(0, 2, 4, 1, 3, 5) ? 0 : 1;
    }
    
    {
        glm::mat3x4 m(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
        glm::mat4x3 t = glm::transpose(m);
        Error += t == glm::mat4x3(0, 4, 8, 1, 5, 9, 2, 6, 10, 3, 7, 11) ? 0 : 1;
    }
    
    {
        glm::mat4 m(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        glm::mat4 t = glm::transpose(m);
        Error += t == glm::mat4(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15) ? 0 : 1;
    }
    
    {
        glm::mat4x2 m(0, 1, 2, 3, 4, 5, 6, 7);
        glm::mat2x4 t = glm::transpose(m);
        Error += t == glm::mat2x4(0, 2, 4, 6, 1, 3, 5, 7) ? 0 : 1;
    }
    
    {
        glm::mat4x3 m(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
        glm::mat3x4 t = glm::transpose(m);
        Error += t == glm::mat3x4(0, 3, 6, 9, 1, 4, 7, 10, 2, 5, 8, 11) ? 0 : 1;
    }
    
	return Error;
}

int test_determinant()
{


	return 0;
}

int test_inverse()
{
	int Failed(0);

	glm::mat4x4 A4x4(
		glm::vec4(1, 0, 1, 0), 
		glm::vec4(0, 1, 0, 0), 
		glm::vec4(0, 0, 1, 0), 
		glm::vec4(0, 0, 0, 1));
	glm::mat4x4 B4x4 = glm::inverse(A4x4);
	glm::mat4x4 I4x4 = A4x4 * B4x4;
	Failed += I4x4 == glm::mat4x4(1) ? 0 : 1;

	glm::mat3x3 A3x3(
		glm::vec3(1, 0, 1), 
		glm::vec3(0, 1, 0), 
		glm::vec3(0, 0, 1));
	glm::mat3x3 B3x3 = glm::inverse(A3x3);
	glm::mat3x3 I3x3 = A3x3 * B3x3;
	Failed += I3x3 == glm::mat3x3(1) ? 0 : 1;

	glm::mat2x2 A2x2(
		glm::vec2(1, 1), 
		glm::vec2(0, 1));
	glm::mat2x2 B2x2 = glm::inverse(A2x2);
	glm::mat2x2 I2x2 = A2x2 * B2x2;
	Failed += I2x2 == glm::mat2x2(1) ? 0 : 1;

	return Failed;
}


int main()
{
	int Failed = 0;
	Failed += test_matrixCompMult();
	Failed += test_outerProduct();
	Failed += test_transpose();
	Failed += test_determinant();
	Failed += test_inverse();
	return Failed;
}

