//-----------------------------------------------------------------------------
//           Name: SIMD.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#pragma once
/*
    SIMD.h

    A suite of functions for working with single-instruction-multiple-data intrinsics
    across various platforms. Floating point vectors only for now but integer vectors
    could be supported later if need be.

TYPES
    typename vec4                : Float vector type, a 16 byte vector containing 4 floats.

LOAD/STORE
    vload(ptr,byteoffset)        : Load with byte offset. Final address must be 16 byte aligned.
                                  Offset must be constant.
    vstore(val,ptr,byteoffset)    : Store with byte offset. Final address must be 16-byte aligned.
                                  Offset must be constant.
    vset(a,b,c,d)                : Set an immediate constant, a,b,c,d must be constants.
    vzero()                        : Return a vector of zeros.
    vone()                        : Return a vector of ones.

MANIPULATION
    vsplat(P,i)                    : Component splat, replicates a single component across a vector. Index must be constant.
                                  returns { P[i], P[i], P[i], P[i] }
    vshuffle(P,a,b,c,d)            : Shuffles the order of the components of P. Indices a,b,c,d must be constant.
                                  returns { P[a], P[b], P[c], P[d] }
    vmergeh(P,Q)                : Merges the first two (or "high") components of each vector into a new one.
                                  returns { P[0], Q[0], P[1], Q[1] }
    vmergel(P,Q)                : Merges the second two (or "low") components of each vector into a new one.
                                  returns { P[2], Q[2], P[3], Q[3] }
    vperm(P,Q, a,b,c,d)            : Pairs two components of P with two components of Q. Indices a,b,c,d must be constant.
                                  returns { P[a], P[b], Q[c], Q[d] }

ARITHMETIC
    vadd(a,b)                    : a + b                addition
    vsub(a,b)                    : a - b                subtraction
    vmul(a,b)                    : a * b                multiplication
    vmadd(a,b,c)                : (a * b) + c        fused multiply-add, often faster than separate add & mul
    vmsub(a,b,c)                : c - (a * b)        fused multiply-subtract, often faster than separate sub & mul
    vdiv(a,b)                    : a / b                division, usually slower than mul
    vrecip(a)                    : 1 / a                reciprocal
    vsqrt(a)                    : sqrt(a)            square root, typically slower than vrsqrt
    vrsqrt(a)                    : 1.0 / sqrt(a)        reciprocal square root, typically faster than vsqrt
    vmin(a,b)                    : a < b ? a : b        minimum
    vmax(a,b)                    : a > b ? a : b        maximum
    vabs(a)                        : a > 0 ? a : -a    absolute value

BIT LOGIC
    vand(a,b)                    : a & b                bitwise and
    vandc(a,b)                    : a & ~b            bitwise and with complement
    vor(a,b)                    : a | b                bitwise or
    vxor(a,b)                    : a ^ b                bitwise exclusive or
    vnot(a)                        : ~a                bitwise complement

COMPARISON
    vequal(a,b)                    : a == b ? ~0 : 0    per-component equal to
    vless(a,b)                    : a < b ? ~0 : 0    per-component less than
    vlessequal(a,b)                : a <= b ? ~0 : 0    per-component less than or equal to
    vgreater(a,b)                : a > b ? ~0 : 0    per-component greater than
    vgreaterequal(a,b)            : a >= b ? ~0 : 0    per-component greater than or equal to

TODO:
    - ps3 implementation
    - vec4 name conflict?
*/

#ifndef CPR_SIMD_H
#define CPR_SIMD_H

namespace SIMD {

#ifdef _WIN32
#define CPR_WINDOWS
#endif
#ifdef __APPLE__
#define CPR_OSX
#endif

/*
inline vec4        vdot4( vec4 a, vec4 b )
{
    vec4 t0 = vmul( a, b );                //x,y,z,w
    vec4 t1 = vshuffle( t0, 3,2,1,0 );    //w,z,y,x
    t0 = vadd( t0, t1 );                //x+w, y+z, z+y, w+x
    t1 = vshuffle( t0, 1,0,3,2 );        //y+z, x+w, w+x, z+y
    t0 = vadd( t0, t1 );                //x+w+y+z, y+z+x+w, z+y+w+x, w+x+z+y
    return t0;
}

inline vec4        vdot3( vec4 a, vec4 b )
{
    vec4 t0 = vmul( a, b );                //x,y,z,w
    vec4 t1 = vshuffle( t0, 2,0,1,3 );    //z,x,y,w
    t1 = vadd( t0, t1 );                //x+z, y+x, z+y, w+w
    t0 = vshuffle( t0, 1,2,0,3 );        //y,z,x,w
    t0 = vadd( t0, t1 );                //x+z+y, y+x+z, z+y+x, w+w+w
    t0 = vsplat( t0, 0 );                //x+z+y, x+z+y, x+z+y, x+z+y
    return t0;
}
*/

//#include "Platform.h"

#if defined(CPR_OSX) && defined(__APPLE_ALTIVEC__)

/*
    Apple Altivec implementation
*/

typedef vector float vec4;

#define vload(ptr, byteoffset) vec_ld(byteoffset, ptr)
#define vstore(val, ptr, byteoffset) vec_st(val, byteoffset, ptr)
#define vset(a, b, c, d) ((vec4)(a, b, c, d))
#define vzero() ((vec4)vec_splat_u32(0))
#define vone() ((vec4)vec_splat_u32(0x3F800000))

#define vsplat(P, i) vec_splat(P, i)
#define vshuffle(P, a, b, c, d) vec_perm(P, vzero(),                                                            \
                                         (vector unsigned char)(4 * (a), 4 * (a) + 1, 4 * (a) + 2, 4 * (a) + 3, \
                                                                4 * (b), 4 * (b) + 1, 4 * (b) + 2, 4 * (b) + 3, \
                                                                4 * (c), 4 * (c) + 1, 4 * (c) + 2, 4 * (c) + 3, \
                                                                4 * (d), 4 * (d) + 1, 4 * (d) + 2, 4 * (d) + 3))
#define vmergeh(P, Q) vec_mergeh(P, Q)
#define vmergel(P, Q) vec_mergel(P, Q)
#define vperm(P, Q, a, b, c, d) vec_perm(P, Q,                                                                          \
                                         (vector unsigned char)(4 * (a), 4 * (a) + 1, 4 * (a) + 2, 4 * (a) + 3,         \
                                                                4 * (b), 4 * (b) + 1, 4 * (b) + 2, 4 * (b) + 3,         \
                                                                4 * (c) + 16, 4 * (c) + 17, 4 * (c) + 18, 4 * (c) + 19, \
                                                                4 * (d) + 16, 4 * (d) + 17, 4 * (d) + 18, 4 * (d) + 19))

#define vadd(a, b) vec_add(a, b)
#define vsub(a, b) vec_sub(a, b)
#define vmul(a, b) vec_madd(a, b, vzero())
#define vmadd(a, b, c) vec_madd(a, b, c)
#define vmsub(a, b, c) vec_nmsub(a, b, c)
#define vdiv(a, b) vec_madd(a, vec_re(b), vzero())
#define vrecip(a) vec_re(a)
#define vsqrt(a) vec_re(vec_rsqrte(a))
#define vrsqrt(a) vec_rsqrte(a)
#define vmin(a, b) vec_min(a, b)
#define vmax(a, b) vec_max(a, b)
#define vabs(a) vec_abs(a)

#define vand(a, b) vec_and(a, b)
#define vandc(a, b) vec_andc(a, b)
#define vor(a, b) vec_or(a, b)
#define vxor(a, b) vec_xor(a, b)
#define vnot(a) vec_andc((vec4)vec_splat_u32(0xFFFFFFFF), a)

#define vequal(a, b) vec_cmpeq(a, b)
#define vless(a, b) vec_cmplt(a, b)
#define vlessequal(a, b) vec_cmple(a, b)
#define vgreater(a, b) vec_cmpgt(a, b)
#define vgreaterequal(a, b) vec_cmpge(a, b)

#elif defined(CPR_WINDOWS) || (defined(CPR_OSX) && !defined(__BIG_ENDIAN__))

/*
    SSE implementation
*/

#include <xmmintrin.h>

typedef __m128 vec4;

#define vload(ptr, byteoffset) _mm_load_ps((const float*)((const char*)(ptr) + (byteoffset)))
#define vstore(val, ptr, byteoffset) _mm_store_ps((float*)((char*)(ptr) + (byteoffset)), val)
#define vset(a, b, c, d) _mm_set_ps(a, b, c, d)
#define vzero() _mm_setzero_ps()
#define vone() _mm_set1_ps(1.0f)

#define vsplat(P, i) vec4_impl::splat<i>(P)
#define vshuffle(P, a, b, c, d) vec4_impl::shuffle<a, b, c, d>(P)
#define vmergeh(P, Q) _mm_unpacklo_ps(P, Q)
#define vmergel(P, Q) _mm_unpackhi_ps(P, Q)
#define vperm(P, Q, a, b, c, d) _mm_shuffle_ps(Q, P, _MM_SHUFFLE((d), (c), (b), (a)))

#define vadd(a, b) _mm_add_ps(a, b)
#define vsub(a, b) _mm_sub_ps(a, b)
#define vmul(a, b) _mm_mul_ps(a, b)
#define vmadd(a, b, c) _mm_add_ps(_mm_mul_ps(a, b), c)
#define vmsub(a, b, c) _mm_sub_ps(c, _mm_mul_ps(a, b))
#define vdiv(a, b) _mm_div_ps(a, b)
#define vrecip(a) _mm_rcp_ps(a)
#define vsqrt(a) _mm_sqrt_ps(a)
#define vrsqrt(a) _mm_rsqrt_ps(a)
#define vmin(a, b) _mm_min_ps(a, b)
#define vmax(a, b) _mm_max_ps(a, b)
#define vabs(a) vec4_impl::abs(a)

#define vand(a, b) _mm_and_ps(a, b)
#define vandc(a, b) _mm_andnot_ps(b, a)
#define vor(a, b) _mm_or_ps(a, b)
#define vxor(a, b) _mm_xor_ps(a, b)
#define vnot(a) vec4_impl::not(a)

#define vequal(a, b) _mm_cmpeq_ps(a, b)
#define vless(a, b) _mm_cmplt_ps(a, b)
#define vlessequal(a, b) _mm_cmple_ps(a, b)
#define vgreater(a, b) _mm_cmpgt_ps(a, b)
#define vgreaterequal(a, b) _mm_cmpge_ps(a, b)

namespace vec4_impl {
template <unsigned Ti>
inline vec4 splat(vec4 P) { return _mm_shuffle_ps(P, P, _MM_SHUFFLE(Ti, Ti, Ti, Ti)); }

template <unsigned Ta, unsigned Tb, unsigned Tc, unsigned Td>
inline vec4 shuffle(vec4 P) { return _mm_shuffle_ps(P, P, _MM_SHUFFLE(Td, Tc, Tb, Ta)); }

inline vec4 abs(vec4 a) { return _mm_max_ps(a, _mm_sub_ps(_mm_setzero_ps(), a)); }

inline vec4 not(vec4 a) { return _mm_andnot_ps(a, _mm_cmpeq_ps(a, a)); }
}  // namespace vec4_impl

#elif defined(CPR_XBOX)

/*
    VMX128 aka "xbox altivec" implementation
*/

#include <PPCIntrinsics.h>

typedef __vector4 vec4;

#define vload(ptr, byteoffset) __lvx(ptr, byteoffset)
#define vstore(val, ptr, byteoffset) __stvx(val, ptr, byteoffset)
#define vset(a, b, c, d) \
    (vec4) { a, b, c, d }
#define vzero() __vspltisw(0)
#define vone() __vspltisw(0x3F800000)

#define vsplat(P, i) __vspltw(P, i)
#define vmergeh(P, Q) __vmrghw(P, Q)
#define vmergel(P, Q) __vmrglw(P, Q)
#define vshuffle(P, a, b, c, d) __vpermwi(P, VPERMWI_CONST(a, b, c, d))
#define vperm(P, Q, a, b, c, d) vec4_impl::perm<a, b, c, d>(P, Q)

#define vadd(a, b) __vaddfp(a, b)
#define vsub(a, b) __vsubfp(a, b)
#define vmul(a, b) __vmulfp(a, b)
#define vmadd(a, b, c) __vmaddfp(a, b, c)
#define vmsub(a, b, c) __vnmsubfp(a, b, c)
#define vdiv(a, b) __vmulfp(a, __vrefp(b))
#define vrecip(a) __vrefp(a)
#define vsqrt(a) __vrefp(__vrsqrtefp(a))
#define vrsqrt(a) __vrsqrtefp(a)
#define vmin(a, b) __vminfp(a, b)
#define vmax(a, b) __vmaxfp(a, b)
#define vabs(a) __vand(a, __vspltisw(0x7FFFFFFF))

#define vand(a, b) __vand(a, b)
#define vandc(a, b) __vandc(a, b)
#define vor(a, b) __vor(a, b)
#define vxor(a, b) __vxor(a, b)
#define vnot(a) __vandc(__vspltisw(0xFFFFFFFF), a);

#define vequal(a, b) __vcmpeqfp(a, b)
#define vless(a, b) __vcmpgtfp(b, a)
#define vlessequal(a, b) __vcmpgefp(b, a)
#define vgreater(a, b) __vcmpgtfp(a, b)
#define vgreaterequal(a, b) __vcmpgefp(a, b)

namespace vec4_impl {
template <unsigned Ta, unsigned Tb, unsigned Tc, unsigned Td>
inline vec4 perm(vec4 P, vec4 Q) {
    P = __vpermwi(P, VPERMWI_CONST(Ta, Tb, 0, 0));
    Q = __vpermwi(Q, VPERMWI_CONST(Tc, Td, 0, 0));
    return __vpermwi(__vmrghw(P, Q), VPERMWI_CONST(0, 2, 1, 3));
}
}  // namespace vec4_impl

#elif defined(CPR_PS3)

/*
    Cell PPU and SPU vector implementation (untested).
*/
#ifdef __SPU__
#include <spu_intrinsics.h>
typedef vec_float4 vec4;
#else
#include <altivec.h>
typedef vector float vec4;
#endif

#ifdef __SPU__

#define vload(ptr, byteoffset) ((const vec4*)((const char*)(ptr) + (byteoffset)))[0]
#define vstore(val, ptr, byteoffset) (((vec4*)((char*)(ptr) + (byteoffset)))[0] = (val))
#define vset(a, b, c, d) ?
#define vzero() spu_splats(0.f)
#define vone() spu_splats(1.f)

#define vsplat(P, i) ?
#define vshuffle(P, a, b, c, d) ?
#define vmergeh(P, Q) ?
#define vmergel(P, Q) ?
#define vperm(P, Q, a, b, c, d) ?

#define vadd(a, b) spu_add(a, b)
#define vsub(a, b) spu_sub(a, b)
#define vmul(a, b) spu_mul(a, b)
#define vmadd(a, b, c) spu_madd(a, b, c)
#define vmsub(a, b, c) spu_nmsub(a, b, c)
#define vdiv(a, b) spu_mul(a, spu_re(b))
#define vrecip(a) spu_re(a)
#define vsqrt(a) spu_re(spu_rsqrte(a))
#define vrsqrt(a) spu_rsqrte(a)
#define vmin(a, b) ?
#define vmax(a, b) ?
#define vabs(a) ?

#define vand(a, b) spu_and(a, b)
#define vandc(a, b) spu_andc(a, b)
#define vor(a, b) spu_or(a, b)
#define vxor(a, b) spu_xor(a, b)
#define vnot(a) spu_not(a)

#define vequal(a, b) spu_cmpeq(a, b)
#define vless(a, b) spu_cmpgt(b, a)
#define vlessequal(a, b) ?
#define vgreater(a, b) spu_cmpgt(a, b)
#define vgreaterequal(a, b) ?

#else

#define vload(ptr, byteoffset) vec_ld(byteoffset, ptr)
#define vstore(val, ptr, byteoffset) vec_st(val, byteoffset, ptr)
#define vset(a, b, c, d) ((vec4)(a, b, c, d))
#define vzero() ((vec4)vec_splat_u32(0))
#define vone() ((vec4)vec_splat_u32(0x3F800000))

#define vsplat(P, i) vec_splat(P, i)
#define vshuffle(P, a, b, c, d) vec_perm(P, vzero(),                                                            \
                                         (vector unsigned char)(4 * (a), 4 * (a) + 1, 4 * (a) + 2, 4 * (a) + 3, \
                                                                4 * (b), 4 * (b) + 1, 4 * (b) + 2, 4 * (b) + 3, \
                                                                4 * (c), 4 * (c) + 1, 4 * (c) + 2, 4 * (c) + 3, \
                                                                4 * (d), 4 * (d) + 1, 4 * (d) + 2, 4 * (d) + 3))
#define vmergeh(P, Q) vec_mergeh(P, Q)
#define vmergel(P, Q) vec_mergel(P, Q)
#define vperm(P, Q, a, b, c, d) vec_perm(P, Q,                                                                          \
                                         (vector unsigned char)(4 * (a), 4 * (a) + 1, 4 * (a) + 2, 4 * (a) + 3,         \
                                                                4 * (b), 4 * (b) + 1, 4 * (b) + 2, 4 * (b) + 3,         \
                                                                4 * (c) + 16, 4 * (c) + 17, 4 * (c) + 18, 4 * (c) + 19, \
                                                                4 * (d) + 16, 4 * (d) + 17, 4 * (d) + 18, 4 * (d) + 19))

#define vadd(a, b) vec_add(a, b)
#define vsub(a, b) vec_sub(a, b)
#define vmul(a, b) vec_madd(a, b, vzero())
#define vmadd(a, b, c) vec_madd(a, b, c)
#define vmsub(a, b, c) vec_nmsub(a, b, c)
#define vdiv(a, b) vec_madd(a, vec_re(b), vzero())
#define vrecip(a) vec_re(a)
#define vsqrt(a) vec_re(vec_rsqrte(a))
#define vrsqrt(a) vec_rsqrte(a)
#define vmin(a, b) vec_min(a, b)
#define vmax(a, b) vec_max(a, b)
#define vabs(a) vec_abs(a)

#define vand(a, b) vec_and(a, b)
#define vandc(a, b) vec_andc(a, b)
#define vor(a, b) vec_or(a, b)
#define vxor(a, b) vec_xor(a, b)
#define vnot(a) vec_andc((vec4)vec_splat_u32(0xFFFFFFFF), a)

#define vequal(a, b) vec_cmpeq(a, b)
#define vless(a, b) vec_cmplt(a, b)
#define vlessequal(a, b) vec_cmple(a, b)
#define vgreater(a, b) vec_cmpgt(a, b)
#define vgreaterequal(a, b) vec_cmpge(a, b)

#endif

#else
#warning Using non-accelerated SIMD instructions
/*
    Scalar implementation.
    Slow, but just here for reference & non-accelerated platforms.
*/

#include <math.h>

typedef union {
    float f[4];
    int i[4];
} vec4;

inline vec4 vload(const void* ptr, long byteoffset) {
    const float* mem = (const float*)((const char*)ptr + byteoffset);
    vec4 r;
    r.f[0] = mem[0];
    r.f[1] = mem[1];
    r.f[2] = mem[2];
    r.f[3] = mem[3];
    return r;
}

inline void vstore(const vec4& val, void* ptr, long byteoffset) {
    float* mem = (float*)((char*)ptr + byteoffset);
    mem[0] = val.f[0];
    mem[1] = val.f[1];
    mem[2] = val.f[2];
    mem[3] = val.f[3];
}

inline vec4 vset(float a, float b, float c, float d) {
    vec4 r;
    r.f[0] = a;
    r.f[1] = b;
    r.f[2] = c;
    r.f[3] = d;
    return r;
}

inline vec4 vzero(void) {
    vec4 r;
    r.f[0] = 0.f;
    r.f[1] = 0.f;
    r.f[2] = 0.f;
    r.f[3] = 0.f;
    return r;
}

inline vec4 vsplat(const vec4& val, unsigned i) {
    vec4 r;
    r.f[0] = r.f[1] = r.f[2] = r.f[3] = val.f[i];
    return r;
}

inline vec4 vshuffle(const vec4& P, unsigned a, unsigned b, unsigned c, unsigned d) {
    vec4 r;
    r.f[0] = P.f[a];
    r.f[1] = P.f[b];
    r.f[2] = P.f[c];
    r.f[3] = P.f[d];
    return r;
}

inline vec4 vmergeh(const vec4& P, const vec4& Q) {
    vec4 r;
    r.f[0] = P.f[0];
    r.f[1] = Q.f[0];
    r.f[2] = P.f[1];
    r.f[3] = Q.f[1];
    return r;
}

inline vec4 vmergel(const vec4& P, const vec4& Q) {
    vec4 r;
    r.f[0] = P.f[2];
    r.f[1] = Q.f[2];
    r.f[2] = P.f[3];
    r.f[3] = Q.f[3];
    return r;
}

inline vec4 vperm(const vec4& P, const vec4& Q, unsigned a, unsigned b, unsigned c, unsigned d) {
    vec4 r;
    r.f[0] = P.f[a];
    r.f[1] = P.f[b];
    r.f[2] = Q.f[c];
    r.f[3] = Q.f[d];
    return r;
}

inline vec4 vadd(const vec4& a, const vec4& b) {
    vec4 r;
    r.f[0] = a.f[0] + b.f[0];
    r.f[1] = a.f[1] + b.f[1];
    r.f[2] = a.f[2] + b.f[2];
    r.f[3] = a.f[3] + b.f[3];
    return r;
}

inline vec4 vsub(const vec4& a, const vec4& b) {
    vec4 r;
    r.f[0] = a.f[0] - b.f[0];
    r.f[1] = a.f[1] - b.f[1];
    r.f[2] = a.f[2] - b.f[2];
    r.f[3] = a.f[3] - b.f[3];
    return r;
}

inline vec4 vmul(const vec4& a, const vec4& b) {
    vec4 r;
    r.f[0] = a.f[0] * b.f[0];
    r.f[1] = a.f[1] * b.f[1];
    r.f[2] = a.f[2] * b.f[2];
    r.f[3] = a.f[3] * b.f[3];
    return r;
}

inline vec4 vmadd(const vec4& a, const vec4& b, const vec4& c) {
    vec4 r;
    r.f[0] = a.f[0] * b.f[0] + c.f[0];
    r.f[1] = a.f[1] * b.f[1] + c.f[1];
    r.f[2] = a.f[2] * b.f[2] + c.f[2];
    r.f[3] = a.f[3] * b.f[3] + c.f[3];
    return r;
}

inline vec4 vmsub(const vec4& a, const vec4& b, const vec4& c) {
    vec4 r;
    r.f[0] = c.f[0] - a.f[0] * b.f[0];
    r.f[1] = c.f[1] - a.f[1] * b.f[1];
    r.f[2] = c.f[2] - a.f[2] * b.f[2];
    r.f[3] = c.f[3] - a.f[3] * b.f[3];
    return r;
}

inline vec4 vdiv(const vec4& a, const vec4& b) {
    vec4 r;
    r.f[0] = a.f[0] / b.f[0];
    r.f[1] = a.f[1] / b.f[1];
    r.f[2] = a.f[2] / b.f[2];
    r.f[3] = a.f[3] / b.f[3];
    return r;
}

inline vec4 vrecip(const vec4& a) {
    vec4 r;
    r.f[0] = 1.f / a.f[0];
    r.f[1] = 1.f / a.f[1];
    r.f[2] = 1.f / a.f[2];
    r.f[3] = 1.f / a.f[3];
    return r;
}

inline vec4 vsqrt(const vec4& a) {
    vec4 r;
    r.f[0] = sqrtf(a.f[0]);
    r.f[1] = sqrtf(a.f[1]);
    r.f[2] = sqrtf(a.f[2]);
    r.f[3] = sqrtf(a.f[3]);
    return r;
}

inline vec4 vrsqrt(const vec4& a) {
    vec4 r;
    r.f[0] = 1.f / sqrtf(a.f[0]);
    r.f[1] = 1.f / sqrtf(a.f[1]);
    r.f[2] = 1.f / sqrtf(a.f[2]);
    r.f[3] = 1.f / sqrtf(a.f[3]);
    return r;
}

inline vec4 vmin(const vec4& a, const vec4& b) {
    vec4 r;
    for (int i = 0; i < 4; ++i) {
        float aa = a.f[i];
        float bb = b.f[i];
        r.f[i] = aa < bb ? aa : bb;
    }
    return r;
}

inline vec4 vmax(const vec4& a, const vec4& b) {
    vec4 r;
    for (int i = 0; i < 4; ++i) {
        float aa = a.f[i];
        float bb = b.f[i];
        r.f[i] = aa > bb ? aa : bb;
    }
    return r;
}

inline vec4 vand(const vec4& a, const vec4 b) {
    vec4 r;
    r.i[0] = a.i[0] & b.i[0];
    r.i[1] = a.i[1] & b.i[1];
    r.i[2] = a.i[2] & b.i[2];
    r.i[3] = a.i[3] & b.i[3];
    return r;
}

inline vec4 vandc(const vec4& a, const vec4 b) {
    vec4 r;
    r.i[0] = a.i[0] & ~b.i[0];
    r.i[1] = a.i[1] & ~b.i[1];
    r.i[2] = a.i[2] & ~b.i[2];
    r.i[3] = a.i[3] & ~b.i[3];
    return r;
}

inline vec4 vor(const vec4& a, const vec4 b) {
    vec4 r;
    r.i[0] = a.i[0] | b.i[0];
    r.i[1] = a.i[1] | b.i[1];
    r.i[2] = a.i[2] | b.i[2];
    r.i[3] = a.i[3] | b.i[3];
    return r;
}

inline vec4 vxor(const vec4& a, const vec4 b) {
    vec4 r;
    r.i[0] = a.i[0] ^ b.i[0];
    r.i[1] = a.i[1] ^ b.i[1];
    r.i[2] = a.i[2] ^ b.i[2];
    r.i[3] = a.i[3] ^ b.i[3];
    return r;
}

inline vec4 vnot(const vec4& a) {
    vec4 r;
    r.i[0] = ~a.i[0];
    r.i[1] = ~a.i[1];
    r.i[2] = ~a.i[2];
    r.i[3] = ~a.i[3];
    return r;
}

inline vec4 vequal(const vec4& a, const vec4& b) {
    vec4 r;
    r.i[0] = a.i[0] == b.i[0] ? 0xFFFFFFFF : 0;
    r.i[1] = a.i[1] == b.i[1] ? 0xFFFFFFFF : 0;
    r.i[2] = a.i[2] == b.i[2] ? 0xFFFFFFFF : 0;
    r.i[3] = a.i[3] == b.i[3] ? 0xFFFFFFFF : 0;
    return r;
}

inline vec4 vless(const vec4& a, const vec4& b) {
    vec4 r;
    r.i[0] = a.i[0] < b.i[0] ? 0xFFFFFFFF : 0;
    r.i[1] = a.i[1] < b.i[1] ? 0xFFFFFFFF : 0;
    r.i[2] = a.i[2] < b.i[2] ? 0xFFFFFFFF : 0;
    r.i[3] = a.i[3] < b.i[3] ? 0xFFFFFFFF : 0;
    return r;
}

inline vec4 vlessequal(const vec4& a, const vec4& b) {
    vec4 r;
    r.i[0] = a.i[0] <= b.i[0] ? 0xFFFFFFFF : 0;
    r.i[1] = a.i[1] <= b.i[1] ? 0xFFFFFFFF : 0;
    r.i[2] = a.i[2] <= b.i[2] ? 0xFFFFFFFF : 0;
    r.i[3] = a.i[3] <= b.i[3] ? 0xFFFFFFFF : 0;
    return r;
}

inline vec4 vgreater(const vec4& a, const vec4& b) {
    return vless(b, a);
}

inline vec4 vgreaterequal(const vec4& a, const vec4& b) {
    return vlessequal(b, a);
}
#endif

}  // namespace SIMD
#endif
