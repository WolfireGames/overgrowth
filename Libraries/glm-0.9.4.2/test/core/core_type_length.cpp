///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-05-25
// Updated : 2011-05-25
// Licence : This source is under MIT License
// File    : test/core/type_length.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>
#include <glm/gtc/half_float.hpp>

int test_length_mat_non_squared()
{
	int Error = 0;

	Error += glm::mat2x3().length() == 2 ? 0 : 1;
	Error += glm::mat2x4().length() == 2 ? 0 : 1;
	Error += glm::mat3x2().length() == 3 ? 0 : 1;
	Error += glm::mat3x4().length() == 3 ? 0 : 1;
	Error += glm::mat4x2().length() == 4 ? 0 : 1;
	Error += glm::mat4x3().length() == 4 ? 0 : 1;
	
	Error += glm::dmat2x3().length() == 2 ? 0 : 1;
	Error += glm::dmat2x4().length() == 2 ? 0 : 1;
	Error += glm::dmat3x2().length() == 3 ? 0 : 1;
	Error += glm::dmat3x4().length() == 3 ? 0 : 1;
	Error += glm::dmat4x2().length() == 4 ? 0 : 1;
	Error += glm::dmat4x3().length() == 4 ? 0 : 1;
	
	Error += glm::hmat2x3().length() == 2 ? 0 : 1;
	Error += glm::hmat2x4().length() == 2 ? 0 : 1;
	Error += glm::hmat3x2().length() == 3 ? 0 : 1;
	Error += glm::hmat3x4().length() == 3 ? 0 : 1;
	Error += glm::hmat4x2().length() == 4 ? 0 : 1;
	Error += glm::hmat4x3().length() == 4 ? 0 : 1;
	
	return Error;
}

int test_length_mat()
{
	int Error = 0;
	
	Error += glm::mat2().length() == 2 ? 0 : 1;
	Error += glm::mat3().length() == 3 ? 0 : 1;
	Error += glm::mat4().length() == 4 ? 0 : 1;
	Error += glm::mat2x2().length() == 2 ? 0 : 1;
	Error += glm::mat3x3().length() == 3 ? 0 : 1;
	Error += glm::mat4x4().length() == 4 ? 0 : 1;
	
	Error += glm::dmat2().length() == 2 ? 0 : 1;
	Error += glm::dmat3().length() == 3 ? 0 : 1;
	Error += glm::dmat4().length() == 4 ? 0 : 1;
	Error += glm::dmat2x2().length() == 2 ? 0 : 1;
	Error += glm::dmat3x3().length() == 3 ? 0 : 1;
	Error += glm::dmat4x4().length() == 4 ? 0 : 1;
	
	Error += glm::hmat2().length() == 2 ? 0 : 1;
	Error += glm::hmat3().length() == 3 ? 0 : 1;
	Error += glm::hmat4().length() == 4 ? 0 : 1;
	Error += glm::hmat2x2().length() == 2 ? 0 : 1;
	Error += glm::hmat3x3().length() == 3 ? 0 : 1;
	Error += glm::hmat4x4().length() == 4 ? 0 : 1;
	
	return Error;
}

int test_length_vec()
{

	int Error = 0;
	
	Error += glm::vec2().length() == 2 ? 0 : 1;
	Error += glm::vec3().length() == 3 ? 0 : 1;
	Error += glm::vec4().length() == 4 ? 0 : 1;

	Error += glm::ivec2().length() == 2 ? 0 : 1;
	Error += glm::ivec3().length() == 3 ? 0 : 1;
	Error += glm::ivec4().length() == 4 ? 0 : 1;

	Error += glm::uvec2().length() == 2 ? 0 : 1;
	Error += glm::uvec3().length() == 3 ? 0 : 1;
	Error += glm::uvec4().length() == 4 ? 0 : 1;

	Error += glm::hvec2().length() == 2 ? 0 : 1;
	Error += glm::hvec3().length() == 3 ? 0 : 1;
	Error += glm::hvec4().length() == 4 ? 0 : 1;
	
	Error += glm::dvec2().length() == 2 ? 0 : 1;
	Error += glm::dvec3().length() == 3 ? 0 : 1;
	Error += glm::dvec4().length() == 4 ? 0 : 1;
	
	return Error;
}

int main()
{
	int Error = 0;
	
	Error += test_length_vec();
	Error += test_length_mat();
	Error += test_length_mat_non_squared();
	
	return Error;
}

