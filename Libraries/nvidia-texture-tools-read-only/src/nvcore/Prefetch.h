// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_PREFETCH_H
#define NV_CORE_PREFETCH_H

#include <nvcore/nvcore.h>

// nvPrefetch
#if NV_CC_GNUC

#define nvPrefetch(ptr)	__builtin_prefetch(ptr)

#elif NV_CC_MSVC 

// Uses SSE Intrinsics for both x86 and x86_64
#include <xmmintrin.h>

__forceinline void nvPrefetch(const void * mem)
{
	_mm_prefetch(static_cast<const char*>(mem), _MM_HINT_T0);	/* prefetcht0  */
//	_mm_prefetch(static_cast<const char*>(mem), _MM_HINT_NTA);	/* prefetchnta */
}
#else

// do nothing in other case.
#define nvPrefetch(ptr)

#endif // NV_CC_MSVC

#endif // NV_CORE_PREFETCH_H
