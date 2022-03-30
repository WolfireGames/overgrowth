// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include <math.h>
#include <float.h> // FLT_MAX

#include "CudaMath.h"

#include "../SingleColorLookup.h"

#define NUM_THREADS 64		// Number of threads per block.

#if __DEVICE_EMULATION__
#define __debugsync() __syncthreads()
#else
#define __debugsync()
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

template <class T> 
__device__ inline void swap(T & a, T & b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

__constant__ float3 kColorMetric = { 1.0f, 1.0f, 1.0f };
__constant__ float3 kColorMetricSqr = { 1.0f, 1.0f, 1.0f };

// Some kernels read the input through texture.
texture<uchar4, 2, cudaReadModeNormalizedFloat> tex;


////////////////////////////////////////////////////////////////////////////////
// Sort colors
////////////////////////////////////////////////////////////////////////////////
__device__ void sortColors(const float * values, int * ranks)
{
#if __DEVICE_EMULATION__
    if (threadIdx.x == 0)
    {
        for (int tid = 0; tid < 16; tid++)
        {
            int rank = 0;
            for (int i = 0; i < 16; i++)
            {
                rank += (values[i] < values[tid]);
            }
            
            ranks[tid] = rank;
        }

        // Resolve elements with the same index.
        for (int i = 0; i < 15; i++)
        {
            for (int tid = 0; tid < 16; tid++)
            {
                if (tid > i && ranks[tid] == ranks[i]) ++ranks[tid];
            }
        }
    }
#else
    const int tid = threadIdx.x;

    int rank = 0;

    #pragma unroll
    for (int i = 0; i < 16; i++)
    {
        rank += (values[i] < values[tid]);
    }
    
    ranks[tid] = rank;

    // Resolve elements with the same index.
    #pragma unroll
    for (int i = 0; i < 15; i++)
    {
        if (tid > i && ranks[tid] == ranks[i]) ++ranks[tid];
    }
#endif
}


////////////////////////////////////////////////////////////////////////////////
// Load color block to shared mem
////////////////////////////////////////////////////////////////////////////////
__device__ void loadColorBlock(const uint * image, float3 colors[16], float3 sums[16], int xrefs[16], int * sameColor)
{
	const int bid = blockIdx.x;
	const int idx = threadIdx.x;

	__shared__ float dps[16];

	if (idx < 16)
	{
		// Read color and copy to shared mem.
		uint c = image[(bid) * 16 + idx];
		
		colors[idx].z = ((c >> 0) & 0xFF) * (1.0f / 255.0f);
		colors[idx].y = ((c >> 8) & 0xFF) * (1.0f / 255.0f);
		colors[idx].x = ((c >> 16) & 0xFF) * (1.0f / 255.0f);
		
		// No need to synchronize, 16 < warp size.
		__debugsync();
		
		// Sort colors along the best fit line.
		colorSums(colors, sums);
		float3 axis = bestFitLine(colors, sums[0], kColorMetric);
		
		*sameColor = (axis == make_float3(0, 0, 0));
		
		dps[idx] = dot(colors[idx], axis);
		
		__debugsync();
		
		sortColors(dps, xrefs);
		
		float3 tmp = colors[idx];
		__debugsync();
		colors[xrefs[idx]] = tmp;
	}
#if __DEVICE_EMULATION__
	else
	{
		__debugsync();
		__debugsync();
		__debugsync();
	}
#endif
}

__device__ void loadColorBlockTex(uint bn, uint w, float3 colors[16], float3 sums[16], int xrefs[16], int * sameColor)
{
	const int bid = blockIdx.x;
	const int idx = threadIdx.x;

	__shared__ float dps[16];

	if (idx < 16)
	{
		float x = 4 * ((bn + bid) % w) + idx % 4;
		float y = 4 * ((bn + bid) / w) + idx / 4;

		// Read color and copy to shared mem.
		float4 c = tex2D(tex, x, y);

		colors[idx].x = c.z;
		colors[idx].y = c.y;
		colors[idx].z = c.x;

		// No need to synchronize, 16 < warp size.
		__debugsync();
		
		// Sort colors along the best fit line.
		colorSums(colors, sums);
		float3 axis = bestFitLine(colors, sums[0], kColorMetric);
		
		*sameColor = (axis == make_float3(0, 0, 0));
		
		dps[idx] = dot(colors[idx], axis);
		
		__debugsync();
		
		sortColors(dps, xrefs);
		
		float3 tmp = colors[idx];
		__debugsync();
		colors[xrefs[idx]] = tmp;
	}
#if __DEVICE_EMULATION__
	else
	{
		__debugsync();
		__debugsync();
		__debugsync();
	}
#endif

}


__device__ void loadColorBlock(const uint * image, float3 colors[16], float3 sums[16], float weights[16], int xrefs[16], int * sameColor)
{
	const int bid = blockIdx.x;
	const int idx = threadIdx.x;

	__shared__ float3 rawColors[16];
	__shared__ float dps[16];

	if (idx < 16)
	{
		// Read color and copy to shared mem.
		uint c = image[(bid) * 16 + idx];
		
		rawColors[idx].z = ((c >> 0) & 0xFF) * (1.0f / 255.0f);
		rawColors[idx].y = ((c >> 8) & 0xFF) * (1.0f / 255.0f);
		rawColors[idx].x = ((c >> 16) & 0xFF) * (1.0f / 255.0f);
		weights[idx] = (((c >> 24) & 0xFF) + 1) * (1.0f / 256.0f);
		
		colors[idx] = rawColors[idx] * weights[idx];

		// No need to synchronize, 16 < warp size.
		__debugsync();

		// Sort colors along the best fit line.
		colorSums(colors, sums);
		float3 axis = bestFitLine(colors, sums[0], kColorMetric);

		*sameColor = (axis == make_float3(0, 0, 0));

		dps[idx] = dot(rawColors[idx], axis);
		
		__debugsync();
		
		sortColors(dps, xrefs);
		
		float3 tmp = colors[idx];
		float w = weights[idx];
		__debugsync();
		colors[xrefs[idx]] = tmp;
		weights[xrefs[idx]] = w;
	}
#if __DEVICE_EMULATION__
	else
	{
		__debugsync();
		__debugsync();
		__debugsync();
	}
#endif
}

__device__ void loadColorBlock(const uint * image, float2 colors[16], float2 sums[16], int xrefs[16], int * sameColor)
{
	const int bid = blockIdx.x;
	const int idx = threadIdx.x;

	__shared__ float dps[16];

	if (idx < 16)
	{
		// Read color and copy to shared mem.
		uint c = image[(bid) * 16 + idx];
	
		colors[idx].y = ((c >> 8) & 0xFF) * (1.0f / 255.0f);
		colors[idx].x = ((c >> 16) & 0xFF) * (1.0f / 255.0f);
		
		// No need to synchronize, 16 < warp size.
		__debugsync();

		// Sort colors along the best fit line.
		colorSums(colors, sums);
		float2 axis = bestFitLine(colors, sums[0]);
		
		*sameColor = (axis == make_float2(0, 0));

		dps[idx] = dot(colors[idx], axis);
		
		__debugsync();
		
		sortColors(dps, xrefs);
		
		float2 tmp = colors[idx];
		__debugsync();
		colors[xrefs[idx]] = tmp;
	}
#if __DEVICE_EMULATION__
	else
	{
		__debugsync();
		__debugsync();
		__debugsync();
	}
#endif
}


////////////////////////////////////////////////////////////////////////////////
// Round color to RGB565 and expand
////////////////////////////////////////////////////////////////////////////////
inline __device__ float3 roundAndExpand565(float3 v, ushort * w)
{
	uint x = __float2uint_rn(__saturatef(v.x) * 31.0f);
	uint y = __float2uint_rn(__saturatef(v.y) * 63.0f);
	uint z = __float2uint_rn(__saturatef(v.z) * 31.0f);
	*w = (x << 11) | (y << 5) | z;
	v.x = __uint2float_rn(x) * 1.0f / 31.0f;
	v.y = __uint2float_rn(y) * 1.0f / 63.0f;
	v.z = __uint2float_rn(z) * 1.0f / 31.0f;
	return v;
}

inline __device__ float2 roundAndExpand56(float2 v, ushort * w)
{
	uint x = __float2uint_rn(__saturatef(v.x) * 31.0f);
	uint y = __float2uint_rn(__saturatef(v.y) * 63.0f);
	*w = (x << 11) | (y << 5);
	v.x = __uint2float_rn(x) * 1.0f / 31.0f;
	v.y = __uint2float_rn(y) * 1.0f / 63.0f;
	return v;
}

inline __device__ float2 roundAndExpand88(float2 v, ushort * w)
{
	uint x = __float2uint_rn(__saturatef(v.x) * 255.0f);
	uint y = __float2uint_rn(__saturatef(v.y) * 255.0f);
	*w = (x << 8) | y;
	v.x = __uint2float_rn(x) * 1.0f / 255.0f;
	v.y = __uint2float_rn(y) * 1.0f / 255.0f;
	return v;
}


////////////////////////////////////////////////////////////////////////////////
// Evaluate permutations
////////////////////////////////////////////////////////////////////////////////
__device__ float evalPermutation4(const float3 * colors, uint permutation, ushort * start, ushort * end)
{
	// Compute endpoints using least squares.
	float alpha2_sum = 0.0f;
	float beta2_sum = 0.0f;
	float alphabeta_sum = 0.0f;
	float3 alphax_sum = make_float3(0.0f, 0.0f, 0.0f);
	float3 betax_sum = make_float3(0.0f, 0.0f, 0.0f);

	// Compute alpha & beta for this permutation.
	for (int i = 0; i < 16; i++)
	{
		const uint bits = permutation >> (2*i);
		
		float beta = (bits & 1);
		if (bits & 2) beta = (1 + beta) / 3.0f;
		float alpha = 1.0f - beta;
		
		alpha2_sum += alpha * alpha;
		beta2_sum += beta * beta;
		alphabeta_sum += alpha * beta;
		alphax_sum += alpha * colors[i];
		betax_sum += beta * colors[i];
	}

	const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

	float3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;
	
	// Round a, b to the closest 5-6-5 color and expand...
	a = roundAndExpand565(a, start);
	b = roundAndExpand565(b, end);

	// compute the error
	float3 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

	return dot(e, kColorMetricSqr);
}

__device__ float evalPermutation3(const float3 * colors, uint permutation, ushort * start, ushort * end)
{
	// Compute endpoints using least squares.
	float alpha2_sum = 0.0f;
	float beta2_sum = 0.0f;
	float alphabeta_sum = 0.0f;
	float3 alphax_sum = make_float3(0.0f, 0.0f, 0.0f);
	float3 betax_sum = make_float3(0.0f, 0.0f, 0.0f);

	// Compute alpha & beta for this permutation.
	for (int i = 0; i < 16; i++)
	{
		const uint bits = permutation >> (2*i);

		float beta = (bits & 1);
		if (bits & 2) beta = 0.5f;
		float alpha = 1.0f - beta;
	
		alpha2_sum += alpha * alpha;
		beta2_sum += beta * beta;
		alphabeta_sum += alpha * beta;
		alphax_sum += alpha * colors[i];
		betax_sum += beta * colors[i];
	}

	const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

	float3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;
	
	// Round a, b to the closest 5-6-5 color and expand...
	a = roundAndExpand565(a, start);
	b = roundAndExpand565(b, end);

	// compute the error
	float3 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

	return dot(e, kColorMetricSqr);
}

__constant__ const float alphaTable4[4] = { 9.0f, 0.0f, 6.0f, 3.0f };
__constant__ const float alphaTable3[4] = { 4.0f, 0.0f, 2.0f, 2.0f };
__constant__ const uint prods4[4] = { 0x090000,0x000900,0x040102,0x010402 };
__constant__ const uint prods3[4] = { 0x040000,0x000400,0x040101,0x010401 };

__device__ float evalPermutation4(const float3 * colors, float3 color_sum, uint permutation, ushort * start, ushort * end)
{
	// Compute endpoints using least squares.
	float3 alphax_sum = make_float3(0.0f, 0.0f, 0.0f);
	uint akku = 0;

	// Compute alpha & beta for this permutation.
	#pragma unroll
	for (int i = 0; i < 16; i++)
	{
		const uint bits = permutation >> (2*i);
		
		alphax_sum += alphaTable4[bits & 3] * colors[i];
		akku += prods4[bits & 3];
	}

	float alpha2_sum = float(akku >> 16);
	float beta2_sum = float((akku >> 8) & 0xff);
	float alphabeta_sum = float(akku & 0xff);
	float3 betax_sum = 9.0f * color_sum - alphax_sum;

	const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

	float3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;
	
	// Round a, b to the closest 5-6-5 color and expand...
	a = roundAndExpand565(a, start);
	b = roundAndExpand565(b, end);

	// compute the error
	float3 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

	return (1.0f / 9.0f) * dot(e, kColorMetricSqr);
}

__device__ float evalPermutation3(const float3 * colors, float3 color_sum, uint permutation, ushort * start, ushort * end)
{
	// Compute endpoints using least squares.
	float3 alphax_sum = make_float3(0.0f, 0.0f, 0.0f);
	uint akku = 0;

	// Compute alpha & beta for this permutation.
	#pragma unroll
	for (int i = 0; i < 16; i++)
	{
		const uint bits = permutation >> (2*i);

		alphax_sum += alphaTable3[bits & 3] * colors[i];
		akku += prods3[bits & 3];
	}

	float alpha2_sum = float(akku >> 16);
	float beta2_sum = float((akku >> 8) & 0xff);
	float alphabeta_sum = float(akku & 0xff);
	float3 betax_sum = 4.0f * color_sum - alphax_sum;

	const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

	float3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;
	
	// Round a, b to the closest 5-6-5 color and expand...
	a = roundAndExpand565(a, start);
	b = roundAndExpand565(b, end);

	// compute the error
	float3 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

	return (1.0f / 4.0f) * dot(e, kColorMetricSqr);
}

__device__ float evalPermutation4(const float3 * colors, const float * weights, float3 color_sum, uint permutation, ushort * start, ushort * end)
{
	// Compute endpoints using least squares.
	float alpha2_sum = 0.0f;
	float beta2_sum = 0.0f;
	float alphabeta_sum = 0.0f;
	float3 alphax_sum = make_float3(0.0f, 0.0f, 0.0f);

	// Compute alpha & beta for this permutation.
	for (int i = 0; i < 16; i++)
	{
		const uint bits = permutation >> (2*i);
		
		float beta = (bits & 1);
		if (bits & 2) beta = (1 + beta) / 3.0f;
		float alpha = 1.0f - beta;
		
		alpha2_sum += alpha * alpha * weights[i];
		beta2_sum += beta * beta * weights[i];
		alphabeta_sum += alpha * beta * weights[i];
		alphax_sum += alpha * colors[i];
	}

	float3 betax_sum = color_sum - alphax_sum;

	const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

	float3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;
	
	// Round a, b to the closest 5-6-5 color and expand...
	a = roundAndExpand565(a, start);
	b = roundAndExpand565(b, end);

	// compute the error
	float3 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

	return dot(e, kColorMetricSqr);
}

/*
__device__ float evalPermutation3(const float3 * colors, const float * weights, uint permutation, ushort * start, ushort * end)
{
	// Compute endpoints using least squares.
	float alpha2_sum = 0.0f;
	float beta2_sum = 0.0f;
	float alphabeta_sum = 0.0f;
	float3 alphax_sum = make_float3(0.0f, 0.0f, 0.0f);

	// Compute alpha & beta for this permutation.
	for (int i = 0; i < 16; i++)
	{
		const uint bits = permutation >> (2*i);

		float beta = (bits & 1);
		if (bits & 2) beta = 0.5f;
		float alpha = 1.0f - beta;

		alpha2_sum += alpha * alpha * weights[i];
		beta2_sum += beta * beta * weights[i];
		alphabeta_sum += alpha * beta * weights[i];
		alphax_sum += alpha * colors[i];
	}

	float3 betax_sum = color_sum - alphax_sum;

	const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

	float3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

	// Round a, b to the closest 5-6-5 color and expand...
	a = roundAndExpand565(a, start);
	b = roundAndExpand565(b, end);

	// compute the error
	float3 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

	return dot(e, kColorMetricSqr);
}
*/

__device__ float evalPermutation4(const float2 * colors, float2 color_sum, uint permutation, ushort * start, ushort * end)
{
	// Compute endpoints using least squares.
	float2 alphax_sum = make_float2(0.0f, 0.0f);
	uint akku = 0;

	// Compute alpha & beta for this permutation.
	#pragma unroll
	for (int i = 0; i < 16; i++)
	{
		const uint bits = permutation >> (2*i);
		
		alphax_sum += alphaTable4[bits & 3] * colors[i];
		akku += prods4[bits & 3];
	}

	float alpha2_sum = float(akku >> 16);
	float beta2_sum = float((akku >> 8) & 0xff);
	float alphabeta_sum = float(akku & 0xff);
	float2 betax_sum = 9.0f * color_sum - alphax_sum;

	const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

	float2 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float2 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;
	
	// Round a, b to the closest 5-6 color and expand...
	a = roundAndExpand56(a, start);
	b = roundAndExpand56(b, end);

	// compute the error
	float2 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

	return (1.0f / 9.0f) * (e.x + e.y);
}

__device__ float evalPermutation3(const float2 * colors, float2 color_sum, uint permutation, ushort * start, ushort * end)
{
	// Compute endpoints using least squares.
	float2 alphax_sum = make_float2(0.0f, 0.0f);
	uint akku = 0;

	// Compute alpha & beta for this permutation.
	#pragma unroll
	for (int i = 0; i < 16; i++)
	{
		const uint bits = permutation >> (2*i);

		alphax_sum += alphaTable3[bits & 3] * colors[i];
		akku += prods3[bits & 3];
	}

	float alpha2_sum = float(akku >> 16);
	float beta2_sum = float((akku >> 8) & 0xff);
	float alphabeta_sum = float(akku & 0xff);
	float2 betax_sum = 4.0f * color_sum - alphax_sum;

	const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

	float2 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float2 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;
	
	// Round a, b to the closest 5-6 color and expand...
	a = roundAndExpand56(a, start);
	b = roundAndExpand56(b, end);

	// compute the error
	float2 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

	return (1.0f / 4.0f) * (e.x + e.y);
}

__device__ float evalPermutationCTX(const float2 * colors, float2 color_sum, uint permutation, ushort * start, ushort * end)
{
	// Compute endpoints using least squares.
	float2 alphax_sum = make_float2(0.0f, 0.0f);
	uint akku = 0;

	// Compute alpha & beta for this permutation.
	#pragma unroll
	for (int i = 0; i < 16; i++)
	{
		const uint bits = permutation >> (2*i);
		
		alphax_sum += alphaTable4[bits & 3] * colors[i];
		akku += prods4[bits & 3];
	}

	float alpha2_sum = float(akku >> 16);
	float beta2_sum = float((akku >> 8) & 0xff);
	float alphabeta_sum = float(akku & 0xff);
	float2 betax_sum = 9.0f * color_sum - alphax_sum;

	const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

	float2 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float2 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;
	
	// Round a, b to the closest 8-8 color and expand...
	a = roundAndExpand88(a, start);
	b = roundAndExpand88(b, end);

	// compute the error
	float2 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

	return (1.0f / 9.0f) * (e.x + e.y);
}


////////////////////////////////////////////////////////////////////////////////
// Evaluate all permutations
////////////////////////////////////////////////////////////////////////////////
__device__ void evalAllPermutations(const float3 * colors, float3 colorSum, const uint * permutations, ushort & bestStart, ushort & bestEnd, uint & bestPermutation, float * errors)
{
	const int idx = threadIdx.x;
	
	float bestError = FLT_MAX;
	
	__shared__ uint s_permutations[160];

	for(int i = 0; i < 16; i++)
	{
		int pidx = idx + NUM_THREADS * i;
		if (pidx >= 992) break;
		
		ushort start, end;
		uint permutation = permutations[pidx];
		if (pidx < 160) s_permutations[pidx] = permutation;
				
		float error = evalPermutation4(colors, colorSum, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
		}
	}

	if (bestStart < bestEnd)
	{
		swap(bestEnd, bestStart);
		bestPermutation ^= 0x55555555;	// Flip indices.
	}

	for(int i = 0; i < 3; i++)
	{
		int pidx = idx + NUM_THREADS * i;
		if (pidx >= 160) break;
		
		ushort start, end;
		uint permutation = s_permutations[pidx];
		float error = evalPermutation3(colors, colorSum, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
			
			if (bestStart > bestEnd)
			{
				swap(bestEnd, bestStart);
				bestPermutation ^= (~bestPermutation >> 1) & 0x55555555;	// Flip indices.
			}
		}
	}

	errors[idx] = bestError;
}

/*
__device__ void evalAllPermutations(const float3 * colors, const float * weights, const uint * permutations, ushort & bestStart, ushort & bestEnd, uint & bestPermutation, float * errors)
{
	const int idx = threadIdx.x;
	
	float bestError = FLT_MAX;
	
	__shared__ uint s_permutations[160];
	
	for(int i = 0; i < 16; i++)
	{
		int pidx = idx + NUM_THREADS * i;
		if (pidx >= 992) break;
		
		ushort start, end;
		uint permutation = permutations[pidx];
		if (pidx < 160) s_permutations[pidx] = permutation;

		float error = evalPermutation4(colors, weights, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
		}
	}

	if (bestStart < bestEnd)
	{
		swap(bestEnd, bestStart);
		bestPermutation ^= 0x55555555;	// Flip indices.
	}

	for(int i = 0; i < 3; i++)
	{
		int pidx = idx + NUM_THREADS * i;
		if (pidx >= 160) break;
		
		ushort start, end;
		uint permutation = s_permutations[pidx];
		float error = evalPermutation3(colors, weights, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
			
			if (bestStart > bestEnd)
			{
				swap(bestEnd, bestStart);
				bestPermutation ^= (~bestPermutation >> 1) & 0x55555555;	// Flip indices.
			}
		}
	}

	errors[idx] = bestError;
}
*/

__device__ void evalAllPermutations(const float2 * colors, float2 colorSum, const uint * permutations, ushort & bestStart, ushort & bestEnd, uint & bestPermutation, float * errors)
{
	const int idx = threadIdx.x;
	
	float bestError = FLT_MAX;
	
	__shared__ uint s_permutations[160];

	for(int i = 0; i < 16; i++)
	{
		int pidx = idx + NUM_THREADS * i;
		if (pidx >= 992) break;
		
		ushort start, end;
		uint permutation = permutations[pidx];
		if (pidx < 160) s_permutations[pidx] = permutation;
				
		float error = evalPermutation4(colors, colorSum, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
		}
	}

	if (bestStart < bestEnd)
	{
		swap(bestEnd, bestStart);
		bestPermutation ^= 0x55555555;	// Flip indices.
	}

	for(int i = 0; i < 3; i++)
	{
		int pidx = idx + NUM_THREADS * i;
		if (pidx >= 160) break;
		
		ushort start, end;
		uint permutation = s_permutations[pidx];
		float error = evalPermutation3(colors, colorSum, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
			
			if (bestStart > bestEnd)
			{
				swap(bestEnd, bestStart);
				bestPermutation ^= (~bestPermutation >> 1) & 0x55555555;	// Flip indices.
			}
		}
	}

	errors[idx] = bestError;
}

__device__ void evalLevel4Permutations(const float3 * colors, float3 colorSum, const uint * permutations, ushort & bestStart, ushort & bestEnd, uint & bestPermutation, float * errors)
{
	const int idx = threadIdx.x;
	
	float bestError = FLT_MAX;
	
	for(int i = 0; i < 16; i++)
	{
		int pidx = idx + NUM_THREADS * i;
		if (pidx >= 992) break;
		
		ushort start, end;
		uint permutation = permutations[pidx];

		float error = evalPermutation4(colors, colorSum, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
		}
	}

	if (bestStart < bestEnd)
	{
		swap(bestEnd, bestStart);
		bestPermutation ^= 0x55555555;	// Flip indices.
	}

	errors[idx] = bestError;
}

__device__ void evalLevel4Permutations(const float3 * colors, const float * weights, float3 colorSum, const uint * permutations, ushort & bestStart, ushort & bestEnd, uint & bestPermutation, float * errors)
{
	const int idx = threadIdx.x;
	
	float bestError = FLT_MAX;
	
	for(int i = 0; i < 16; i++)
	{
		int pidx = idx + NUM_THREADS * i;
		if (pidx >= 992) break;
		
		ushort start, end;
		uint permutation = permutations[pidx];

		float error = evalPermutation4(colors, weights, colorSum, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
		}
	}

	if (bestStart < bestEnd)
	{
		swap(bestEnd, bestStart);
		bestPermutation ^= 0x55555555;	// Flip indices.
	}

	errors[idx] = bestError;
}

__device__ void evalAllPermutationsCTX(const float2 * colors, float2 colorSum, const uint * permutations, ushort & bestStart, ushort & bestEnd, uint & bestPermutation, float * errors)
{
	const int idx = threadIdx.x;
	
	float bestError = FLT_MAX;
	
	for(int i = 0; i < 16; i++)
	{
		int pidx = idx + NUM_THREADS * i;
		if (pidx >= 704) break;
		
		ushort start, end;
		uint permutation = permutations[pidx];

		float error = evalPermutationCTX(colors, colorSum, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
		}
	}

	if (bestStart < bestEnd)
	{
		swap(bestEnd, bestStart);
		bestPermutation ^= 0x55555555;	// Flip indices.
	}

	errors[idx] = bestError;
}


////////////////////////////////////////////////////////////////////////////////
// Find index with minimum error
////////////////////////////////////////////////////////////////////////////////
__device__ int findMinError(float * errors)
{
	const int idx = threadIdx.x;

	__shared__ int indices[NUM_THREADS];
	indices[idx] = idx;

#if __DEVICE_EMULATION__
	for(int d = NUM_THREADS/2; d > 0; d >>= 1)
	{
		__syncthreads();
		
		if (idx < d)
		{
			float err0 = errors[idx];
			float err1 = errors[idx + d];
			
			if (err1 < err0) {
				errors[idx] = err1;
				indices[idx] = indices[idx + d];
			}
		}
	}
#else
	for(int d = NUM_THREADS/2; d > 32; d >>= 1)
	{
		__syncthreads();
		
		if (idx < d)
		{
			float err0 = errors[idx];
			float err1 = errors[idx + d];
			
			if (err1 < err0) {
				errors[idx] = err1;
				indices[idx] = indices[idx + d];
			}
		}
	}

	__syncthreads();

	// unroll last 6 iterations
	if (idx < 32)
	{
		if (errors[idx + 32] < errors[idx]) {
			errors[idx] = errors[idx + 32];
			indices[idx] = indices[idx + 32];
		}
		if (errors[idx + 16] < errors[idx]) {
			errors[idx] = errors[idx + 16];
			indices[idx] = indices[idx + 16];
		}
		if (errors[idx + 8] < errors[idx]) {
			errors[idx] = errors[idx + 8];
			indices[idx] = indices[idx + 8];
		}
		if (errors[idx + 4] < errors[idx]) {
			errors[idx] = errors[idx + 4];
			indices[idx] = indices[idx + 4];
		}
		if (errors[idx + 2] < errors[idx]) {
			errors[idx] = errors[idx + 2];
			indices[idx] = indices[idx + 2];
		}
		if (errors[idx + 1] < errors[idx]) {
			errors[idx] = errors[idx + 1];
			indices[idx] = indices[idx + 1];
		}
	}
#endif

	__syncthreads();

	return indices[0];
}


////////////////////////////////////////////////////////////////////////////////
// Save DXT block
////////////////////////////////////////////////////////////////////////////////
__device__ void saveBlockDXT1(ushort start, ushort end, uint permutation, int xrefs[16], uint2 * result)
{
	const int bid = blockIdx.x;

	if (start == end)
	{
		permutation = 0;
	}
	
	// Reorder permutation.
	uint indices = 0;
	for(int i = 0; i < 16; i++)
	{
		int ref = xrefs[i];
		indices |= ((permutation >> (2 * ref)) & 3) << (2 * i);
	}
	
	// Write endpoints.
	result[bid].x = (end << 16) | start;
	
	// Write palette indices.
	result[bid].y = indices;
}

__device__ void saveBlockCTX1(ushort start, ushort end, uint permutation, int xrefs[16], uint2 * result)
{
	saveBlockDXT1(start, end, permutation, xrefs, result);
}

__device__ void saveSingleColorBlockDXT1(float3 color, uint2 * result)
{
	const int bid = blockIdx.x;

	int r = color.x * 255;
	int g = color.y * 255;
	int b = color.z * 255;

	ushort color0 = (OMatch5[r][0] << 11) | (OMatch6[g][0] << 5) | OMatch5[b][0];
	ushort color1 = (OMatch5[r][1] << 11) | (OMatch6[g][1] << 5) | OMatch5[b][1];

	if (color0 < color1)
	{
		result[bid].x = (color0 << 16) | color1;
		result[bid].y = 0xffffffff;
	}
	else
	{
		result[bid].x = (color1 << 16) | color0;
		result[bid].y = 0xaaaaaaaa;
	}
}

__device__ void saveSingleColorBlockDXT1(float2 color, uint2 * result)
{
	const int bid = blockIdx.x;

	int r = color.x * 255;
	int g = color.y * 255;

	ushort color0 = (OMatch5[r][0] << 11) | (OMatch6[g][0] << 5);
	ushort color1 = (OMatch5[r][1] << 11) | (OMatch6[g][1] << 5);

	if (color0 < color1)
	{
		result[bid].x = (color0 << 16) | color1;
		result[bid].y = 0xffffffff;
	}
	else
	{
		result[bid].x = (color1 << 16) | color0;
		result[bid].y = 0xaaaaaaaa;
	}
}

__device__ void saveSingleColorBlockCTX1(float2 color, uint2 * result)
{
	const int bid = blockIdx.x;

	int r = color.x * 255;
	int g = color.y * 255;

	ushort color0 = (r << 8) | (g);
	
	result[bid].x = (color0 << 16) | color0;
	result[bid].y = 0x00000000;
}


////////////////////////////////////////////////////////////////////////////////
// Compress color block
////////////////////////////////////////////////////////////////////////////////
__global__ void compressDXT1(const uint * permutations, const uint * image, uint2 * result)
{
	__shared__ float3 colors[16];
	__shared__ float3 sums[16];
	__shared__ int xrefs[16];
	__shared__ int sameColor;
	
	loadColorBlock(image, colors, sums, xrefs, &sameColor);

	__syncthreads();

	if (sameColor)
	{
		if (threadIdx.x == 0) saveSingleColorBlockDXT1(colors[0], result);
		return;
	}

	ushort bestStart, bestEnd;
	uint bestPermutation;

	__shared__ float errors[NUM_THREADS];

	evalAllPermutations(colors, sums[0], permutations, bestStart, bestEnd, bestPermutation, errors);
	
	// Use a parallel reduction to find minimum error.
	const int minIdx = findMinError(errors);
	
	// Only write the result of the winner thread.
	if (threadIdx.x == minIdx)
	{
		saveBlockDXT1(bestStart, bestEnd, bestPermutation, xrefs, result);
	}
}

__global__ void compressDXT1_Tex(uint bn, uint w, const uint * permutations, uint2 * result)
{
	__shared__ float3 colors[16];
	__shared__ float3 sums[16];
	__shared__ int xrefs[16];
	__shared__ int sameColor;
	
	loadColorBlockTex(bn, w, colors, sums, xrefs, &sameColor);

	__syncthreads();

	if (sameColor)
	{
		if (threadIdx.x == 0) saveSingleColorBlockDXT1(colors[0], result);
		return;
	}

	ushort bestStart, bestEnd;
	uint bestPermutation;

	__shared__ float errors[NUM_THREADS];

	evalAllPermutations(colors, sums[0], permutations, bestStart, bestEnd, bestPermutation, errors);
	
	// Use a parallel reduction to find minimum error.
	const int minIdx = findMinError(errors);
	
	// Only write the result of the winner thread.
	if (threadIdx.x == minIdx)
	{
		saveBlockDXT1(bestStart, bestEnd, bestPermutation, xrefs, result);
	}
}


__global__ void compressLevel4DXT1(const uint * permutations, const uint * image, uint2 * result)
{
	__shared__ float3 colors[16];
	__shared__ float3 sums[16];
	__shared__ int xrefs[16];
	__shared__ int sameColor;
	
	loadColorBlock(image, colors, sums, xrefs, &sameColor);

	__syncthreads();

	if (sameColor)
	{
		if (threadIdx.x == 0) saveSingleColorBlockDXT1(colors[0], result);
		return;
	}

	ushort bestStart, bestEnd;
	uint bestPermutation;

	__shared__ float errors[NUM_THREADS];

	evalLevel4Permutations(colors, sums[0], permutations, bestStart, bestEnd, bestPermutation, errors);
	
	// Use a parallel reduction to find minimum error.
	const int minIdx = findMinError(errors);
	
	// Only write the result of the winner thread.
	if (threadIdx.x == minIdx)
	{
		saveBlockDXT1(bestStart, bestEnd, bestPermutation, xrefs, result);
	}
}

__global__ void compressWeightedDXT1(const uint * permutations, const uint * image, uint2 * result)
{
	__shared__ float3 colors[16];
	__shared__ float3 sums[16];
	__shared__ float weights[16];
	__shared__ int xrefs[16];
	__shared__ int sameColor;
	
	loadColorBlock(image, colors, sums, weights, xrefs, &sameColor);
	
	__syncthreads();

	if (sameColor)
	{
		if (threadIdx.x == 0) saveSingleColorBlockDXT1(colors[0], result);
		return;
	}

	ushort bestStart, bestEnd;
	uint bestPermutation;

	__shared__ float errors[NUM_THREADS];
	
	evalLevel4Permutations(colors, weights, sums[0], permutations, bestStart, bestEnd, bestPermutation, errors);
	
	// Use a parallel reduction to find minimum error.
	int minIdx = findMinError(errors);
	
	// Only write the result of the winner thread.
	if (threadIdx.x == minIdx)
	{
		saveBlockDXT1(bestStart, bestEnd, bestPermutation, xrefs, result);
	}
}


__global__ void compressNormalDXT1(const uint * permutations, const uint * image, uint2 * result)
{
	__shared__ float2 colors[16];
	__shared__ float2 sums[16];
	__shared__ int xrefs[16];
	__shared__ int sameColor;

	loadColorBlock(image, colors, sums, xrefs, &sameColor);
	
	__syncthreads();

	if (sameColor)
	{
		if (threadIdx.x == 0) saveSingleColorBlockDXT1(colors[0], result);
		return;
	}

	ushort bestStart, bestEnd;
	uint bestPermutation;

	__shared__ float errors[NUM_THREADS];

	evalAllPermutations(colors, sums[0], permutations, bestStart, bestEnd, bestPermutation, errors);
	
	// Use a parallel reduction to find minimum error.
	const int minIdx = findMinError(errors);
	
	// Only write the result of the winner thread.
	if (threadIdx.x == minIdx)
	{
		saveBlockDXT1(bestStart, bestEnd, bestPermutation, xrefs, result);
	}
}

__global__ void compressCTX1(const uint * permutations, const uint * image, uint2 * result)
{
	__shared__ float2 colors[16];
	__shared__ float2 sums[16];
	__shared__ int xrefs[16];
	__shared__ int sameColor;
	
	loadColorBlock(image, colors, sums, xrefs, &sameColor);
	
	__syncthreads();

	if (sameColor)
	{
		if (threadIdx.x == 0) saveSingleColorBlockCTX1(colors[0], result);
		return;
	}

	ushort bestStart, bestEnd;
	uint bestPermutation;

	__shared__ float errors[NUM_THREADS];

	evalAllPermutationsCTX(colors, sums[0], permutations, bestStart, bestEnd, bestPermutation, errors);
	
	// Use a parallel reduction to find minimum error.
	const int minIdx = findMinError(errors);
	
	// Only write the result of the winner thread.
	if (threadIdx.x == minIdx)
	{
		saveBlockCTX1(bestStart, bestEnd, bestPermutation, xrefs, result);
	}
}


/*
__device__ float computeError(const float weights[16], uchar a0, uchar a1)
{
	float palette[6];
	palette[0] = (6.0f/7.0f * a0 + 1.0f/7.0f * a1);
	palette[1] = (5.0f/7.0f * a0 + 2.0f/7.0f * a1);
	palette[2] = (4.0f/7.0f * a0 + 3.0f/7.0f * a1);
	palette[3] = (3.0f/7.0f * a0 + 4.0f/7.0f * a1);
	palette[4] = (2.0f/7.0f * a0 + 5.0f/7.0f * a1);
	palette[5] = (1.0f/7.0f * a0 + 6.0f/7.0f * a1);

	float total = 0.0f;

	for (uint i = 0; i < 16; i++)
	{
		float alpha = weights[i];

		float error = a0 - alpha;
		error = min(error, palette[0] - alpha);
		error = min(error, palette[1] - alpha);
		error = min(error, palette[2] - alpha);
		error = min(error, palette[3] - alpha);
		error = min(error, palette[4] - alpha);
		error = min(error, palette[5] - alpha);
		error = min(error, a1 - alpha);
		
		total += error;
	}
	
	return total;
}

inline __device__ uchar roundAndExpand(float a)
{
	return rintf(__saturatef(a) * 255.0f);
}
*/
/*
__device__ void optimizeAlpha8(const float alphas[16], uchar & a0, uchar & a1)
{
	float alpha2_sum = 0;
	float beta2_sum = 0;
	float alphabeta_sum = 0;
	float alphax_sum = 0;
	float betax_sum = 0;

	for (int i = 0; i < 16; i++)
	{
		uint idx = index[i];
		float alpha;
		if (idx < 2) alpha = 1.0f - idx;
		else alpha = (8.0f - idx) / 7.0f;
		
		float beta = 1 - alpha;

		alpha2_sum += alpha * alpha;
		beta2_sum += beta * beta;
		alphabeta_sum += alpha * beta;
		alphax_sum += alpha * alphas[i];
		betax_sum += beta * alphas[i];
	}

	const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

	float a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

	a0 = roundAndExpand8(a);
	a1 = roundAndExpand8(b);
}
*/
/*
__device__ void compressAlpha(const float alphas[16], uint4 * result)
{
	const int tid = threadIdx.x;
	
	// Compress alpha block!
	// Brute force approach:
	// Try all color pairs: 256*256/2 = 32768, 32768/64 = 512 iterations?

	// Determine min & max alphas

	float A0, A1;

	if (tid < 16)
	{
		__shared__ uint s_alphas[16];
		
		s_alphas[tid] = alphas[tid];
		s_alphas[tid] = min(s_alphas[tid], s_alphas[tid^8]);
		s_alphas[tid] = min(s_alphas[tid], s_alphas[tid^4]);
		s_alphas[tid] = min(s_alphas[tid], s_alphas[tid^2]);
		s_alphas[tid] = min(s_alphas[tid], s_alphas[tid^1]);
		A0 = s_alphas[tid];
		
		s_alphas[tid] = alphas[tid];
		s_alphas[tid] = max(s_alphas[tid], s_alphas[tid^8]);
		s_alphas[tid] = max(s_alphas[tid], s_alphas[tid^4]);
		s_alphas[tid] = max(s_alphas[tid], s_alphas[tid^2]);
		s_alphas[tid] = max(s_alphas[tid], s_alphas[tid^1]);
		A1 = s_alphas[tid];
	}

	__syncthreads();

	int minIdx = 0;

	if (A1 - A0 > 8)
	{
		float bestError = FLT_MAX;

		// 64 threads -> 8x8
		// divide [A1-A0] in partitions.
		// test endpoints 
		
		for (int i = 0; i < 128; i++)
		{
			uint idx = (i * NUM_THREADS + tid) * 4;
			uchar a0 = idx & 255;
			uchar a1 = idx >> 8;
			
			float error = computeError(alphas, a0, a1);
			
			if (error < bestError)
			{
				bestError = error;
				A0 = a0;
				A1 = a1;
			}
		}
		
		__shared__ float errors[NUM_THREADS];
		errors[tid] = bestError;
		
		// Minimize error.
		minIdx = findMinError(errors);

	}

	if (minIdx == tid)
	{
		// @@ Compute indices.
	
		// @@ Write alpha block.
	}
}

__global__ void compressDXT5(const uint * permutations, const uint * image, uint4 * result)
{
	__shared__ float3 colors[16];
	__shared__ float3 sums[16];
	__shared__ float weights[16];
	__shared__ int xrefs[16];
	
	loadColorBlock(image, colors, sums, weights, xrefs);
	
	__syncthreads();

	compressAlpha(weights, result);	

	ushort bestStart, bestEnd;
	uint bestPermutation;

	__shared__ float errors[NUM_THREADS];
	
	evalLevel4Permutations(colors, weights, sums[0], permutations, bestStart, bestEnd, bestPermutation, errors);
	
	// Use a parallel reduction to find minimum error.
	int minIdx = findMinError(errors);
	
	// Only write the result of the winner thread.
	if (threadIdx.x == minIdx)
	{
		saveBlockDXT1(bestStart, bestEnd, bestPermutation, xrefs, (uint2 *)result);
	}
}
*/

/*__device__ void evaluatePalette(uint alpha0, uint alpha1, uint alphas[8])
{
	alpha[0] = alpha0;
	alpha[1] = alpha1;
	alpha[2] = (6 * alpha[0] + 1 * alpha[1]) / 7;	// bit code 010
	alpha[3] = (5 * alpha[0] + 2 * alpha[1]) / 7;	// bit code 011
	alpha[4] = (4 * alpha[0] + 3 * alpha[1]) / 7;	// bit code 100
	alpha[5] = (3 * alpha[0] + 4 * alpha[1]) / 7;	// bit code 101
	alpha[6] = (2 * alpha[0] + 5 * alpha[1]) / 7;	// bit code 110
	alpha[7] = (1 * alpha[0] + 6 * alpha[1]) / 7;	// bit code 111
}

__device__ uint computeAlphaError(const uint block[16], uint alpha0, uint alpha1, int bestError = INT_MAX)
{
	uint8 alphas[8];
	evaluatePalette(alpha0, alpha1, alphas);

	int totalError = 0;

	for (uint i = 0; i < 16; i++)
	{
		uint8 alpha = block[i];

		// @@ It should be possible to do this much faster.

		int minDist = INT_MAX;
		for (uint p = 0; p < 8; p++)
		{
			int dist = alphaDistance(alpha, alphas[p]);
			minDist = min(dist, minDist);
		}



		totalError += minDist;

		if (totalError > bestError)
		{
			// early out
			return totalError;
		}
	}

	return totalError;
}


void compressDXT5A(uint alpha[16])
{
	// Get min/max alpha.
	for (uint i = 0; i < 16; i++)
	{
		mina = min(mina, alpha[i]);
		maxa = max(maxa, alpha[i]);
	}

	dxtBlock->alpha0 = maxa;
	dxtBlock->alpha1 = mina;

	if (maxa - mina > 8)
	{
		int besterror = computeAlphaError(rgba, dxtBlock);
		int besta0 = maxa;
		int besta1 = mina;

		// Expand search space a bit.
		const int alphaExpand = 8;
		mina = (mina <= alphaExpand) ? 0 : mina - alphaExpand;
		maxa = (maxa <= 255-alphaExpand) ? 255 : maxa + alphaExpand;

		for (int a0 = mina+9; a0 < maxa; a0++)
		{
			for (int a1 = mina; a1 < a0-8; a1++)
			{
				nvDebugCheck(a0 - a1 > 8);

				dxtBlock->alpha0 = a0;
				dxtBlock->alpha1 = a1;
				int error = computeAlphaError(rgba, dxtBlock, besterror);

				if (error < besterror)
				{
					besterror = error;
					besta0 = a0;
					besta1 = a1;
				}
			}
		}

		dxtBlock->alpha0 = besta0;
		dxtBlock->alpha1 = besta1;
	}
}

__global__ void compressDXT5n(uint blockNum, uint2 * d_result)
{
	uint idx = blockIdx.x * 128 + threadIdx.x;

	if (idx >= blockNum)
	{
		return;
	}

	// @@ Ideally we would load the data to shared mem to achieve coalesced global mem access.
	// @@ Blocks would require too much shared memory (8k) and limit occupancy.

	// @@ Ideally we should use SIMD processing, multiple threads (4-8) processing the same block.
	// That simplifies coalescing, and reduces divergence.

	// @@ Experiment with texture. That's probably the most simple approach.

	uint x[16];
	uint y[16];


}
*/


////////////////////////////////////////////////////////////////////////////////
// Setup kernel
////////////////////////////////////////////////////////////////////////////////

extern "C" void setupCompressKernel(const float weights[3])
{
	// Set constants.
	cudaMemcpyToSymbol(kColorMetric, weights, sizeof(float) * 3, 0);

	float weightsSqr[3];
	weightsSqr[0] = weights[0] * weights[0];
	weightsSqr[1] = weights[1] * weights[1];
	weightsSqr[2] = weights[2] * weights[2];

	cudaMemcpyToSymbol(kColorMetricSqr, weightsSqr, sizeof(float) * 3, 0);
}


////////////////////////////////////////////////////////////////////////////////
// Launch kernel
////////////////////////////////////////////////////////////////////////////////

extern "C" void compressKernelDXT1(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps)
{
	compressDXT1<<<blockNum, NUM_THREADS>>>(d_bitmaps, d_data, (uint2 *)d_result);
}

extern "C" void compressKernelDXT1_Tex(uint bn, uint blockNum, uint w, cudaArray * d_data, uint * d_result, uint * d_bitmaps)
{
	// Setup texture
	tex.normalized = false;
	tex.filterMode = cudaFilterModePoint;
	tex.addressMode[0] = cudaAddressModeClamp;
	tex.addressMode[1] = cudaAddressModeClamp;
    
	cudaBindTextureToArray(tex, d_data);

	compressDXT1_Tex<<<blockNum, NUM_THREADS>>>(bn, w, d_bitmaps, (uint2 *)d_result);
}


extern "C" void compressKernelDXT1_Level4(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps)
{
	compressLevel4DXT1<<<blockNum, NUM_THREADS>>>(d_bitmaps, d_data, (uint2 *)d_result);
}

extern "C" void compressWeightedKernelDXT1(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps)
{
	compressWeightedDXT1<<<blockNum, NUM_THREADS>>>(d_bitmaps, d_data, (uint2 *)d_result);
}

extern "C" void compressNormalKernelDXT1(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps)
{
	compressNormalDXT1<<<blockNum, NUM_THREADS>>>(d_bitmaps, d_data, (uint2 *)d_result);
}

extern "C" void compressKernelCTX1(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps)
{
	compressCTX1<<<blockNum, NUM_THREADS>>>(d_bitmaps, d_data, (uint2 *)d_result);
}

extern "C" void compressKernelDXT5n(uint blockNum, cudaArray * d_data, uint * d_result)
{
	// Setup texture
	tex.normalized = false;
	tex.filterMode = cudaFilterModePoint;
	tex.addressMode[0] = cudaAddressModeClamp;
	tex.addressMode[1] = cudaAddressModeClamp;
    
	cudaBindTextureToArray(tex, d_data);

//	compressDXT5n<<<blockNum/128, 128>>>(blockNum, (uint2 *)d_result);
}
