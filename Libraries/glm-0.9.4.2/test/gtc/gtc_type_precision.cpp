///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-09-16
// Updated : 2011-05-08
// Licence : This source is under MIT licence
// File    : test/gtc/type_precision.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>
#include <glm/gtc/type_precision.hpp>

static int test_scalar_size()
{
	int Error = 0;
    Error += sizeof(glm::int8) != 1;
    Error += sizeof(glm::int16) != 2;
    Error += sizeof(glm::int32) != 4;
    Error += sizeof(glm::int64) != 8;
    Error += sizeof(glm::uint8) != 1;
    Error += sizeof(glm::uint16) != 2;
    Error += sizeof(glm::uint32) != 4;
    Error += sizeof(glm::uint64) != 8;
    Error += sizeof(glm::float16) != 2;
    Error += sizeof(glm::float32) != 4;
    Error += sizeof(glm::float64) != 8;
	return Error;
}

static int test_fvec_size()
{
    int Error = 0;
    Error += sizeof(glm::f16vec2) != 4;
    Error += sizeof(glm::f16vec3) != 6;
    Error += sizeof(glm::f16vec4) != 8;
    Error += sizeof(glm::f32vec2) != 8;
    Error += sizeof(glm::f32vec3) != 12;
    Error += sizeof(glm::f32vec4) != 16;
    Error += sizeof(glm::f64vec2) != 16;
    Error += sizeof(glm::f64vec3) != 24;
    Error += sizeof(glm::f64vec4) != 32;
	return Error;
}

static int test_ivec_size()
{
    int Error = 0;
    Error += sizeof(glm::i8vec2) != 2;
    Error += sizeof(glm::i8vec3) != 3;
    Error += sizeof(glm::i8vec4) != 4;
    Error += sizeof(glm::i16vec2) != 4;
    Error += sizeof(glm::i16vec3) != 6;
    Error += sizeof(glm::i16vec4) != 8;
    Error += sizeof(glm::i32vec2) != 8;
    Error += sizeof(glm::i32vec3) != 12;
    Error += sizeof(glm::i32vec4) != 16;
    Error += sizeof(glm::i64vec2) != 16;
    Error += sizeof(glm::i64vec3) != 24;
    Error += sizeof(glm::i64vec4) != 32;
	return Error;
}

static int test_uvec_size()
{
    int Error = 0;
    Error += sizeof(glm::u8vec2) != 2;
    Error += sizeof(glm::u8vec3) != 3;
    Error += sizeof(glm::u8vec4) != 4;
    Error += sizeof(glm::u16vec2) != 4;
    Error += sizeof(glm::u16vec3) != 6;
    Error += sizeof(glm::u16vec4) != 8;
    Error += sizeof(glm::u32vec2) != 8;
    Error += sizeof(glm::u32vec3) != 12;
    Error += sizeof(glm::u32vec4) != 16;
    Error += sizeof(glm::u64vec2) != 16;
    Error += sizeof(glm::u64vec3) != 24;
    Error += sizeof(glm::u64vec4) != 32;
	return Error;
}

static int test_hmat_size()
{
    int Error = 0;
    Error += sizeof(glm::f16mat2) != 8;
    Error += sizeof(glm::f16mat3) != 18;
    Error += sizeof(glm::f16mat4) != 32;
    Error += sizeof(glm::f16mat2x2) != 8;
    Error += sizeof(glm::f16mat2x3) != 12;
    Error += sizeof(glm::f16mat2x4) != 16;
    Error += sizeof(glm::f16mat3x2) != 12;
    Error += sizeof(glm::f16mat3x3) != 18;
    Error += sizeof(glm::f16mat3x4) != 24;
    Error += sizeof(glm::f16mat4x2) != 16;
    Error += sizeof(glm::f16mat4x3) != 24;
    Error += sizeof(glm::f16mat4x4) != 32;
	return Error;
}

static int test_fmat_size()
{
    int Error = 0;
    Error += sizeof(glm::fmat2) != 16;
    Error += sizeof(glm::fmat3) != 36;
    Error += sizeof(glm::fmat4) != 64;
    Error += sizeof(glm::fmat2x2) != 16;
    Error += sizeof(glm::fmat2x3) != 24;
    Error += sizeof(glm::fmat2x4) != 32;
    Error += sizeof(glm::fmat3x2) != 24;
    Error += sizeof(glm::fmat3x3) != 36;
    Error += sizeof(glm::fmat3x4) != 48;
    Error += sizeof(glm::fmat4x2) != 32;
    Error += sizeof(glm::fmat4x3) != 48;
    Error += sizeof(glm::fmat4x4) != 64;

    Error += sizeof(glm::f32mat2) != 16;
    Error += sizeof(glm::f32mat3) != 36;
    Error += sizeof(glm::f32mat4) != 64;
    Error += sizeof(glm::f32mat2x2) != 16;
    Error += sizeof(glm::f32mat2x3) != 24;
    Error += sizeof(glm::f32mat2x4) != 32;
    Error += sizeof(glm::f32mat3x2) != 24;
    Error += sizeof(glm::f32mat3x3) != 36;
    Error += sizeof(glm::f32mat3x4) != 48;
    Error += sizeof(glm::f32mat4x2) != 32;
    Error += sizeof(glm::f32mat4x3) != 48;
    Error += sizeof(glm::f32mat4x4) != 64;
	return Error;
}

static int test_dmat_size()
{
    int Error = 0;
    Error += sizeof(glm::f64mat2) != 32;
    Error += sizeof(glm::f64mat3) != 72;
    Error += sizeof(glm::f64mat4) != 128;
    Error += sizeof(glm::f64mat2x2) != 32;
    Error += sizeof(glm::f64mat2x3) != 48;
    Error += sizeof(glm::f64mat2x4) != 64;
    Error += sizeof(glm::f64mat3x2) != 48;
    Error += sizeof(glm::f64mat3x3) != 72;
    Error += sizeof(glm::f64mat3x4) != 96;
    Error += sizeof(glm::f64mat4x2) != 64;
    Error += sizeof(glm::f64mat4x3) != 96;
    Error += sizeof(glm::f64mat4x4) != 128;
	return Error;
}

static int test_quat_size()
{
    int Error = 0;
    Error += sizeof(glm::f16quat) != 8; 
    Error += sizeof(glm::f32quat) != 16; 
    Error += sizeof(glm::f64quat) != 32;
	return Error;
}

int main()
{
	int Error = 0;
    Error += test_scalar_size();
	Error += test_fvec_size();
    Error += test_ivec_size();
	Error += test_uvec_size();
	Error += test_hmat_size();
	Error += test_fmat_size();
	Error += test_dmat_size();
	Error += test_quat_size();
	return Error;
}
