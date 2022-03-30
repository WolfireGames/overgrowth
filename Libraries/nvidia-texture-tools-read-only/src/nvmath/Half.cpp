// Branch-free implementation of half-precision (16 bit) floating point
// Copyright 2006 Mike Acton <macton@gmail.com>
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE
//
// Half-precision floating point format
// ------------------------------------
//
//   | Field    | Last | First | Note
//   |----------|------|-------|----------
//   | Sign     | 15   | 15    |
//   | Exponent | 14   | 10    | Bias = 15
//   | Mantissa | 9    | 0     |
//
// Compiling
// ---------
//
//  Preferred compile flags for GCC: 
//     -O3 -fstrict-aliasing -std=c99 -pedantic -Wall -Wstrict-aliasing
//
//     This file is a C99 source file, intended to be compiled with a C99 
//     compliant compiler. However, for the moment it remains combatible
//     with C++98. Therefore if you are using a compiler that poorly implements
//     C standards (e.g. MSVC), it may be compiled as C++. This is not
//     guaranteed for future versions. 
//
// Features
// --------
//
//  * QNaN + <x>  = QNaN
//  * <x>  + +INF = +INF
//  * <x>  - -INF = -INF
//  * INF  - INF  = SNaN
//  * Denormalized values
//  * Difference of ZEROs is always +ZERO
//  * Sum round with guard + round + sticky bit (grs)
//  * And of course... no branching
// 
// Precision of Sum
// ----------------
//
//  (SUM)        uint16 z = half_add( x, y );
//  (DIFFERENCE) uint16 z = half_add( x, -y );
//
//     Will have exactly (0 ulps difference) the same result as:
//     (For 32 bit IEEE 784 floating point and same rounding mode)
//
//     union FLOAT_32
//     {
//       float    f32;
//       uint32 u32;
//     };
//
//     union FLOAT_32 fx = { .u32 = half_to_float( x ) };
//     union FLOAT_32 fy = { .u32 = half_to_float( y ) };
//     union FLOAT_32 fz = { .f32 = fx.f32 + fy.f32    };
//     uint16       z  = float_to_half( fz );
//

#include "Half.h"
#include <stdio.h>

// Load immediate
static inline uint32 _uint32_li( uint32 a )
{
  return (a);
}

// Decrement
static inline uint32 _uint32_dec( uint32 a )
{
  return (a - 1);
}

// Complement
static inline uint32 _uint32_not( uint32 a )
{
  return (~a);
}

// Negate
static inline uint32 _uint32_neg( uint32 a )
{
#if NV_CC_MSVC
  // prevent msvc warning.
  return ~a + 1;
#else
  return (-a);
#endif
}

// Extend sign
static inline uint32 _uint32_ext( uint32 a )
{
  return (((int32)a)>>31);
}

// And
static inline uint32 _uint32_and( uint32 a, uint32 b )
{
  return (a & b);
}

// And with Complement
static inline uint32 _uint32_andc( uint32 a, uint32 b )
{
  return (a & ~b);
}

// Or
static inline uint32 _uint32_or( uint32 a, uint32 b )
{
  return (a | b);
}

// Shift Right Logical
static inline uint32 _uint32_srl( uint32 a, int sa )
{
  return (a >> sa);
}

// Shift Left Logical
static inline uint32 _uint32_sll( uint32 a, int sa )
{
  return (a << sa);
}

// Add
static inline uint32 _uint32_add( uint32 a, uint32 b )
{
  return (a + b);
}

// Subtract
static inline uint32 _uint32_sub( uint32 a, uint32 b )
{
  return (a - b);
}

// Select on Sign bit
static inline uint32 _uint32_sels( uint32 test, uint32 a, uint32 b )
{
  const uint32 mask   = _uint32_ext( test );
  const uint32 sel_a  = _uint32_and(  a,     mask  );
  const uint32 sel_b  = _uint32_andc( b,     mask  );
  const uint32 result = _uint32_or(   sel_a, sel_b );

  return (result);
}

// Load Immediate
static inline uint16 _uint16_li( uint16 a )
{
  return (a);
}

// Extend sign
static inline uint16 _uint16_ext( uint16 a )
{
  return (((int16)a)>>15);
}

// Negate
static inline uint16 _uint16_neg( uint16 a )
{
  return (-a);
}

// Complement
static inline uint16 _uint16_not( uint16 a )
{
  return (~a);
}

// Decrement
static inline uint16 _uint16_dec( uint16 a )
{
  return (a - 1);
}

// Shift Left Logical
static inline uint16 _uint16_sll( uint16 a, int sa )
{
  return (a << sa);
}

// Shift Right Logical
static inline uint16 _uint16_srl( uint16 a, int sa )
{
  return (a >> sa);
}

// Add
static inline uint16 _uint16_add( uint16 a, uint16 b )
{
  return (a + b);
}

// Subtract
static inline uint16 _uint16_sub( uint16 a, uint16 b )
{
  return (a - b);
}

// And
static inline uint16 _uint16_and( uint16 a, uint16 b )
{
  return (a & b);
}

// Or
static inline uint16 _uint16_or( uint16 a, uint16 b )
{
  return (a | b);
}

// Exclusive Or
static inline uint16 _uint16_xor( uint16 a, uint16 b )
{
  return (a ^ b);
}

// And with Complement
static inline uint16 _uint16_andc( uint16 a, uint16 b )
{
  return (a & ~b);
}

// And then Shift Right Logical
static inline uint16 _uint16_andsrl( uint16 a, uint16 b, int sa )
{
  return ((a & b) >> sa);
}

// Shift Right Logical then Mask
static inline uint16 _uint16_srlm( uint16 a, int sa, uint16 mask )
{
  return ((a >> sa) & mask);
}

// Add then Mask
static inline uint16 _uint16_addm( uint16 a, uint16 b, uint16 mask )
{
  return ((a + b) & mask);
}


// Select on Sign bit
static inline uint16 _uint16_sels( uint16 test, uint16 a, uint16 b )
{
  const uint16 mask   = _uint16_ext( test );
  const uint16 sel_a  = _uint16_and(  a,     mask  );
  const uint16 sel_b  = _uint16_andc( b,     mask  );
  const uint16 result = _uint16_or(   sel_a, sel_b );

  return (result);
}

// Count Leading Zeros
static inline uint32 _uint32_cntlz( uint32 x )
{
#ifdef __GNUC__
  /* On PowerPC, this will map to insn: cntlzw */
  /* On Pentium, this will map to insn: clz    */
  uint32 nlz = __builtin_clz( x );
  return (nlz);
#else
  const uint32 x0  = _uint32_srl(  x,  1 );
  const uint32 x1  = _uint32_or(   x,  x0 );
  const uint32 x2  = _uint32_srl(  x1, 2 );
  const uint32 x3  = _uint32_or(   x1, x2 );
  const uint32 x4  = _uint32_srl(  x3, 4 );
  const uint32 x5  = _uint32_or(   x3, x4 );
  const uint32 x6  = _uint32_srl(  x5, 8 );
  const uint32 x7  = _uint32_or(   x5, x6 );
  const uint32 x8  = _uint32_srl(  x7, 16 );
  const uint32 x9  = _uint32_or(   x7, x8 );
  const uint32 xA  = _uint32_not(  x9 );
  const uint32 xB  = _uint32_srl(  xA, 1 );
  const uint32 xC  = _uint32_and(  xB, 0x55555555 );
  const uint32 xD  = _uint32_sub(  xA, xC );
  const uint32 xE  = _uint32_and(  xD, 0x33333333 );
  const uint32 xF  = _uint32_srl(  xD, 2 );
  const uint32 x10 = _uint32_and(  xF, 0x33333333 );
  const uint32 x11 = _uint32_add(  xE, x10 );
  const uint32 x12 = _uint32_srl(  x11, 4 );
  const uint32 x13 = _uint32_add(  x11, x12 );
  const uint32 x14 = _uint32_and(  x13, 0x0f0f0f0f );
  const uint32 x15 = _uint32_srl(  x14, 8 );
  const uint32 x16 = _uint32_add(  x14, x15 );
  const uint32 x17 = _uint32_srl(  x16, 16 );
  const uint32 x18 = _uint32_add(  x16, x17 );
  const uint32 x19 = _uint32_and(  x18, 0x0000003f );
  return ( x19 );
#endif
}

// Count Leading Zeros
static inline uint16 _uint16_cntlz( uint16 x )
{
#ifdef __GNUC__
  /* On PowerPC, this will map to insn: cntlzw */
  /* On Pentium, this will map to insn: clz    */
  uint32 x32   = _uint32_sll( x, 16 );
  uint16 nlz   = (uint16)__builtin_clz( x32 );
  return (nlz);
#else
  const uint16 x0  = _uint16_srl(  x,  1 );
  const uint16 x1  = _uint16_or(   x,  x0 );
  const uint16 x2  = _uint16_srl(  x1, 2 );
  const uint16 x3  = _uint16_or(   x1, x2 );
  const uint16 x4  = _uint16_srl(  x3, 4 );
  const uint16 x5  = _uint16_or(   x3, x4 );
  const uint16 x6  = _uint16_srl(  x5, 8 );
  const uint16 x7  = _uint16_or(   x5, x6 );
  const uint16 x8  = _uint16_not(  x7 );
  const uint16 x9  = _uint16_srlm( x8, 1, 0x5555 );
  const uint16 xA  = _uint16_sub(  x8, x9 );
  const uint16 xB  = _uint16_and(  xA, 0x3333 );
  const uint16 xC  = _uint16_srlm( xA, 2, 0x3333 );
  const uint16 xD  = _uint16_add(  xB, xC );
  const uint16 xE  = _uint16_srl(  xD, 4 );
  const uint16 xF  = _uint16_addm( xD, xE, 0x0f0f );
  const uint16 x10 = _uint16_srl(  xF, 8 );
  const uint16 x11 = _uint16_addm( xF, x10, 0x001f );
  return ( x11 );
#endif
}

uint16
half_from_float( uint32 f )
{
  const uint32 one                        = _uint32_li( 0x00000001 );
  const uint32 f_e_mask                   = _uint32_li( 0x7f800000 );
  const uint32 f_m_mask                   = _uint32_li( 0x007fffff );
  const uint32 f_s_mask                   = _uint32_li( 0x80000000 );
  const uint32 h_e_mask                   = _uint32_li( 0x00007c00 );
  const uint32 f_e_pos                    = _uint32_li( 0x00000017 );
  const uint32 f_m_round_bit              = _uint32_li( 0x00001000 );
  const uint32 h_nan_em_min               = _uint32_li( 0x00007c01 );
  const uint32 f_h_s_pos_offset           = _uint32_li( 0x00000010 );
  const uint32 f_m_hidden_bit             = _uint32_li( 0x00800000 );
  const uint32 f_h_m_pos_offset           = _uint32_li( 0x0000000d );
  const uint32 f_h_bias_offset            = _uint32_li( 0x38000000 );
  const uint32 f_m_snan_mask              = _uint32_li( 0x003fffff );
  const uint16 h_snan_mask                = _uint32_li( 0x00007e00 );
  const uint32 f_e                        = _uint32_and( f, f_e_mask  );
  const uint32 f_m                        = _uint32_and( f, f_m_mask  );
  const uint32 f_s                        = _uint32_and( f, f_s_mask  );
  const uint32 f_e_h_bias                 = _uint32_sub( f_e,               f_h_bias_offset );
  const uint32 f_e_h_bias_amount          = _uint32_srl( f_e_h_bias,        f_e_pos         );
  const uint32 f_m_round_mask             = _uint32_and( f_m,               f_m_round_bit     );
  const uint32 f_m_round_offset           = _uint32_sll( f_m_round_mask,    one               );
  const uint32 f_m_rounded                = _uint32_add( f_m,               f_m_round_offset  );
  const uint32 f_m_rounded_overflow       = _uint32_and( f_m_rounded,       f_m_hidden_bit    );
  const uint32 f_m_denorm_sa              = _uint32_sub( one,               f_e_h_bias_amount );
  const uint32 f_m_with_hidden            = _uint32_or(  f_m_rounded,       f_m_hidden_bit    );
  const uint32 f_m_denorm                 = _uint32_srl( f_m_with_hidden,   f_m_denorm_sa     );
  const uint32 f_em_norm_packed           = _uint32_or(  f_e_h_bias,        f_m_rounded       );
  const uint32 f_e_overflow               = _uint32_add( f_e_h_bias,        f_m_hidden_bit    );
  const uint32 h_s                        = _uint32_srl( f_s,               f_h_s_pos_offset );
  const uint32 h_m_nan                    = _uint32_srl( f_m,               f_h_m_pos_offset );
  const uint32 h_m_denorm                 = _uint32_srl( f_m_denorm,        f_h_m_pos_offset );
  const uint32 h_em_norm                  = _uint32_srl( f_em_norm_packed,  f_h_m_pos_offset );
  const uint32 h_em_overflow              = _uint32_srl( f_e_overflow,      f_h_m_pos_offset );
  const uint32 is_e_eqz_msb               = _uint32_dec(  f_e     );
  const uint32 is_m_nez_msb               = _uint32_neg(  f_m     );
  const uint32 is_h_m_nan_nez_msb         = _uint32_neg(  h_m_nan );
  const uint32 is_e_nflagged_msb          = _uint32_sub(  f_e,                 f_e_mask          );
  const uint32 is_ninf_msb                = _uint32_or(   is_e_nflagged_msb,   is_m_nez_msb      );
  const uint32 is_underflow_msb           = _uint32_sub(  is_e_eqz_msb,        f_h_bias_offset   );
  const uint32 is_nan_nunderflow_msb      = _uint32_or(   is_h_m_nan_nez_msb,  is_e_nflagged_msb );
  const uint32 is_m_snan_msb              = _uint32_sub(  f_m_snan_mask,       f_m               );
  const uint32 is_snan_msb                = _uint32_andc( is_m_snan_msb,       is_e_nflagged_msb );
  const uint32 is_overflow_msb            = _uint32_neg(  f_m_rounded_overflow );
  const uint32 h_nan_underflow_result     = _uint32_sels( is_nan_nunderflow_msb, h_em_norm,                h_nan_em_min       );
  const uint32 h_inf_result               = _uint32_sels( is_ninf_msb,           h_nan_underflow_result,   h_e_mask           );
  const uint32 h_underflow_result         = _uint32_sels( is_underflow_msb,      h_m_denorm,               h_inf_result       );
  const uint32 h_overflow_result          = _uint32_sels( is_overflow_msb,       h_em_overflow,            h_underflow_result );
  const uint32 h_em_result                = _uint32_sels( is_snan_msb,           h_snan_mask,              h_overflow_result  );
  const uint32 h_result                   = _uint32_or( h_em_result, h_s );

  return (h_result);
}

uint32 
half_to_float( uint16 h )
{
  const uint32 h_e_mask              = _uint32_li( 0x00007c00 );
  const uint32 h_m_mask              = _uint32_li( 0x000003ff );
  const uint32 h_s_mask              = _uint32_li( 0x00008000 );
  const uint32 h_f_s_pos_offset      = _uint32_li( 0x00000010 );
  const uint32 h_f_e_pos_offset      = _uint32_li( 0x0000000d );
  const uint32 h_f_bias_offset       = _uint32_li( 0x0001c000 );
  const uint32 f_e_mask              = _uint32_li( 0x7f800000 );
  const uint32 f_m_mask              = _uint32_li( 0x007fffff );
  const uint32 h_f_e_denorm_bias     = _uint32_li( 0x0000007e );
  const uint32 h_f_m_denorm_sa_bias  = _uint32_li( 0x00000008 );
  const uint32 f_e_pos               = _uint32_li( 0x00000017 );
  const uint32 h_e_mask_minus_one    = _uint32_li( 0x00007bff );
  const uint32 h_e                   = _uint32_and( h, h_e_mask );
  const uint32 h_m                   = _uint32_and( h, h_m_mask );
  const uint32 h_s                   = _uint32_and( h, h_s_mask );
  const uint32 h_e_f_bias            = _uint32_add( h_e, h_f_bias_offset );
  const uint32 h_m_nlz               = _uint32_cntlz( h_m );
  const uint32 f_s                   = _uint32_sll( h_s,        h_f_s_pos_offset );
  const uint32 f_e                   = _uint32_sll( h_e_f_bias, h_f_e_pos_offset );
  const uint32 f_m                   = _uint32_sll( h_m,        h_f_e_pos_offset );
  const uint32 f_em                  = _uint32_or(  f_e,        f_m              );
  const uint32 h_f_m_sa              = _uint32_sub( h_m_nlz,             h_f_m_denorm_sa_bias );
  const uint32 f_e_denorm_unpacked   = _uint32_sub( h_f_e_denorm_bias,   h_f_m_sa             );
  const uint32 h_f_m                 = _uint32_sll( h_m,                 h_f_m_sa             );
  const uint32 f_m_denorm            = _uint32_and( h_f_m,               f_m_mask             );
  const uint32 f_e_denorm            = _uint32_sll( f_e_denorm_unpacked, f_e_pos              );
  const uint32 f_em_denorm           = _uint32_or(  f_e_denorm,          f_m_denorm           );
  const uint32 f_em_nan              = _uint32_or(  f_e_mask,            f_m                  );
  const uint32 is_e_eqz_msb          = _uint32_dec(  h_e );
  const uint32 is_m_nez_msb          = _uint32_neg(  h_m );
  const uint32 is_e_flagged_msb      = _uint32_sub(  h_e_mask_minus_one, h_e );
  const uint32 is_zero_msb           = _uint32_andc( is_e_eqz_msb,       is_m_nez_msb );
  const uint32 is_inf_msb            = _uint32_andc( is_e_flagged_msb,   is_m_nez_msb );
  const uint32 is_denorm_msb         = _uint32_and(  is_m_nez_msb,       is_e_eqz_msb );
  const uint32 is_nan_msb            = _uint32_and(  is_e_flagged_msb,   is_m_nez_msb ); 
  const uint32 is_zero               = _uint32_ext(  is_zero_msb );
  const uint32 f_zero_result         = _uint32_andc( f_em, is_zero );
  const uint32 f_denorm_result       = _uint32_sels( is_denorm_msb, f_em_denorm, f_zero_result );
  const uint32 f_inf_result          = _uint32_sels( is_inf_msb,    f_e_mask,    f_denorm_result );
  const uint32 f_nan_result          = _uint32_sels( is_nan_msb,    f_em_nan,    f_inf_result    );
  const uint32 f_result              = _uint32_or( f_s, f_nan_result );
 
  return (f_result);
}

uint16
half_add( uint16 x, uint16 y )
{
  const uint16 one                       = _uint16_li( 0x0001 );
  const uint16 msb_to_lsb_sa             = _uint16_li( 0x000f );
  const uint16 h_s_mask                  = _uint16_li( 0x8000 );
  const uint16 h_e_mask                  = _uint16_li( 0x7c00 );
  const uint16 h_m_mask                  = _uint16_li( 0x03ff );
  const uint16 h_m_msb_mask              = _uint16_li( 0x2000 );
  const uint16 h_m_msb_sa                = _uint16_li( 0x000d );
  const uint16 h_m_hidden                = _uint16_li( 0x0400 );
  const uint16 h_e_pos                   = _uint16_li( 0x000a );
  const uint16 h_e_bias_minus_one        = _uint16_li( 0x000e );
  const uint16 h_m_grs_carry             = _uint16_li( 0x4000 );
  const uint16 h_m_grs_carry_pos         = _uint16_li( 0x000e );
  const uint16 h_grs_size                = _uint16_li( 0x0003 );
  const uint16 h_snan                    = _uint16_li( 0xfe00 );
  const uint16 h_e_mask_minus_one        = _uint16_li( 0x7bff );
  const uint16 h_grs_round_carry         = _uint16_sll( one, h_grs_size );
  const uint16 h_grs_round_mask          = _uint16_sub( h_grs_round_carry, one );
  const uint16 x_e                       = _uint16_and( x, h_e_mask );
  const uint16 y_e                       = _uint16_and( y, h_e_mask );
  const uint16 is_y_e_larger_msb         = _uint16_sub( x_e, y_e );
  const uint16 a                         = _uint16_sels( is_y_e_larger_msb, y, x);
  const uint16 a_s                       = _uint16_and( a, h_s_mask );
  const uint16 a_e                       = _uint16_and( a, h_e_mask );
  const uint16 a_m_no_hidden_bit         = _uint16_and( a, h_m_mask );
  const uint16 a_em_no_hidden_bit        = _uint16_or( a_e, a_m_no_hidden_bit );
  const uint16 b                         = _uint16_sels( is_y_e_larger_msb, x, y);
  const uint16 b_s                       = _uint16_and( b, h_s_mask );
  const uint16 b_e                       = _uint16_and( b, h_e_mask );
  const uint16 b_m_no_hidden_bit         = _uint16_and( b, h_m_mask );
  const uint16 b_em_no_hidden_bit        = _uint16_or( b_e, b_m_no_hidden_bit );
  const uint16 is_diff_sign_msb          = _uint16_xor( a_s, b_s );
  const uint16 is_a_inf_msb              = _uint16_sub( h_e_mask_minus_one, a_em_no_hidden_bit );
  const uint16 is_b_inf_msb              = _uint16_sub( h_e_mask_minus_one, b_em_no_hidden_bit );
  const uint16 is_undenorm_msb           = _uint16_dec( a_e );
  const uint16 is_undenorm               = _uint16_ext( is_undenorm_msb );
  const uint16 is_both_inf_msb           = _uint16_and( is_a_inf_msb, is_b_inf_msb );
  const uint16 is_invalid_inf_op_msb     = _uint16_and( is_both_inf_msb, b_s );
  const uint16 is_a_e_nez_msb            = _uint16_neg( a_e );
  const uint16 is_b_e_nez_msb            = _uint16_neg( b_e );
  const uint16 is_a_e_nez                = _uint16_ext( is_a_e_nez_msb );
  const uint16 is_b_e_nez                = _uint16_ext( is_b_e_nez_msb );
  const uint16 a_m_hidden_bit            = _uint16_and( is_a_e_nez, h_m_hidden );
  const uint16 b_m_hidden_bit            = _uint16_and( is_b_e_nez, h_m_hidden );
  const uint16 a_m_no_grs                = _uint16_or( a_m_no_hidden_bit, a_m_hidden_bit );
  const uint16 b_m_no_grs                = _uint16_or( b_m_no_hidden_bit, b_m_hidden_bit );
  const uint16 diff_e                    = _uint16_sub( a_e,        b_e );
  const uint16 a_e_unbias                = _uint16_sub( a_e,        h_e_bias_minus_one );
  const uint16 a_m                       = _uint16_sll( a_m_no_grs, h_grs_size );
  const uint16 a_e_biased                = _uint16_srl( a_e,        h_e_pos );
  const uint16 m_sa_unbias               = _uint16_srl( a_e_unbias, h_e_pos );
  const uint16 m_sa_default              = _uint16_srl( diff_e,     h_e_pos );
  const uint16 m_sa_unbias_mask          = _uint16_andc( is_a_e_nez_msb,   is_b_e_nez_msb );
  const uint16 m_sa                      = _uint16_sels( m_sa_unbias_mask, m_sa_unbias, m_sa_default );
  const uint16 b_m_no_sticky             = _uint16_sll( b_m_no_grs,        h_grs_size );
  const uint16 sh_m                      = _uint16_srl( b_m_no_sticky,     m_sa );
  const uint16 sticky_overflow           = _uint16_sll( one,               m_sa );
  const uint16 sticky_mask               = _uint16_dec( sticky_overflow );
  const uint16 sticky_collect            = _uint16_and( b_m_no_sticky, sticky_mask );
  const uint16 is_sticky_set_msb         = _uint16_neg( sticky_collect );
  const uint16 sticky                    = _uint16_srl( is_sticky_set_msb, msb_to_lsb_sa);
  const uint16 b_m                       = _uint16_or( sh_m, sticky );
  const uint16 is_c_m_ab_pos_msb         = _uint16_sub( b_m, a_m );
  const uint16 c_inf                     = _uint16_or( a_s, h_e_mask );
  const uint16 c_m_sum                   = _uint16_add( a_m, b_m );
  const uint16 c_m_diff_ab               = _uint16_sub( a_m, b_m );
  const uint16 c_m_diff_ba               = _uint16_sub( b_m, a_m );
  const uint16 c_m_smag_diff             = _uint16_sels( is_c_m_ab_pos_msb, c_m_diff_ab, c_m_diff_ba );
  const uint16 c_s_diff                  = _uint16_sels( is_c_m_ab_pos_msb, a_s,         b_s         );
  const uint16 c_s                       = _uint16_sels( is_diff_sign_msb,  c_s_diff,    a_s         );
  const uint16 c_m_smag_diff_nlz         = _uint16_cntlz( c_m_smag_diff );
  const uint16 diff_norm_sa              = _uint16_sub( c_m_smag_diff_nlz, one );
  const uint16 is_diff_denorm_msb        = _uint16_sub( a_e_biased, diff_norm_sa );
  const uint16 is_diff_denorm            = _uint16_ext( is_diff_denorm_msb );
  const uint16 is_a_or_b_norm_msb        = _uint16_neg( a_e_biased );
  const uint16 diff_denorm_sa            = _uint16_dec( a_e_biased );
  const uint16 c_m_diff_denorm           = _uint16_sll( c_m_smag_diff, diff_denorm_sa );
  const uint16 c_m_diff_norm             = _uint16_sll( c_m_smag_diff, diff_norm_sa );
  const uint16 c_e_diff_norm             = _uint16_sub( a_e_biased,  diff_norm_sa );
  const uint16 c_m_diff_ab_norm          = _uint16_sels( is_diff_denorm_msb, c_m_diff_denorm, c_m_diff_norm );
  const uint16 c_e_diff_ab_norm          = _uint16_andc( c_e_diff_norm, is_diff_denorm );
  const uint16 c_m_diff                  = _uint16_sels( is_a_or_b_norm_msb, c_m_diff_ab_norm, c_m_smag_diff );
  const uint16 c_e_diff                  = _uint16_sels( is_a_or_b_norm_msb, c_e_diff_ab_norm, a_e_biased    );
  const uint16 is_diff_eqz_msb           = _uint16_dec( c_m_diff );
  const uint16 is_diff_exactly_zero_msb  = _uint16_and( is_diff_sign_msb, is_diff_eqz_msb );
  const uint16 is_diff_exactly_zero      = _uint16_ext( is_diff_exactly_zero_msb );
  const uint16 c_m_added                 = _uint16_sels( is_diff_sign_msb, c_m_diff, c_m_sum );
  const uint16 c_e_added                 = _uint16_sels( is_diff_sign_msb, c_e_diff, a_e_biased );
  const uint16 c_m_carry                 = _uint16_and( c_m_added, h_m_grs_carry );
  const uint16 is_c_m_carry_msb          = _uint16_neg( c_m_carry );
  const uint16 c_e_hidden_offset         = _uint16_andsrl( c_m_added, h_m_grs_carry, h_m_grs_carry_pos );
  const uint16 c_m_sub_hidden            = _uint16_srl( c_m_added, one );
  const uint16 c_m_no_hidden             = _uint16_sels( is_c_m_carry_msb, c_m_sub_hidden, c_m_added );
  const uint16 c_e_no_hidden             = _uint16_add( c_e_added,         c_e_hidden_offset  );
  const uint16 c_m_no_hidden_msb         = _uint16_and( c_m_no_hidden,     h_m_msb_mask       );
  const uint16 undenorm_m_msb_odd        = _uint16_srl( c_m_no_hidden_msb, h_m_msb_sa         );
  const uint16 undenorm_fix_e            = _uint16_and( is_undenorm,       undenorm_m_msb_odd );
  const uint16 c_e_fixed                 = _uint16_add( c_e_no_hidden,     undenorm_fix_e     );
  const uint16 c_m_round_amount          = _uint16_and( c_m_no_hidden,     h_grs_round_mask   );
  const uint16 c_m_rounded               = _uint16_add( c_m_no_hidden,     c_m_round_amount   );
  const uint16 c_m_round_overflow        = _uint16_andsrl( c_m_rounded, h_m_grs_carry, h_m_grs_carry_pos );
  const uint16 c_e_rounded               = _uint16_add( c_e_fixed, c_m_round_overflow );
  const uint16 c_m_no_grs                = _uint16_srlm( c_m_rounded, h_grs_size,  h_m_mask );
  const uint16 c_e                       = _uint16_sll( c_e_rounded, h_e_pos );
  const uint16 c_em                      = _uint16_or( c_e, c_m_no_grs );
  const uint16 c_normal                  = _uint16_or( c_s, c_em );
  const uint16 c_inf_result              = _uint16_sels( is_a_inf_msb, c_inf, c_normal );
  const uint16 c_zero_result             = _uint16_andc( c_inf_result, is_diff_exactly_zero );
  const uint16 c_result                  = _uint16_sels( is_invalid_inf_op_msb, h_snan, c_zero_result );

  return (c_result);
}
