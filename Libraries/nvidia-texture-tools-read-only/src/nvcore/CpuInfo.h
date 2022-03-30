// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_CPUINFO_H
#define NV_CORE_CPUINFO_H

#include <nvcore/nvcore.h>

#if NV_CC_MSVC
#if _MSC_VER >= 1400
#	include <intrin.h> // __rdtsc
#endif
#endif


namespace nv
{

	// CPU Information.
	class CpuInfo
	{
	protected:
		static int cpu();

	private:
		// Cache of the CPU data
		static uint m_cpu;
		static uint m_procCount;

	public:
		static uint processorCount();
		static uint coreCount();

		static bool hasMMX();
		static bool hasSSE();
		static bool hasSSE2();
		static bool hasSSE3();
	};

#if NV_CC_MSVC
#if _MSC_VER < 1400
       inline uint64 rdtsc()
        {
		uint64 t;
		__asm rdtsc 
		__asm mov DWORD PTR [t], eax 
		__asm mov DWORD PTR [t+4], edx
		return t;
        }	
#else
	#pragma intrinsic(__rdtsc)

	inline uint64 rdtsc()
	{
		return __rdtsc();
	}
#endif
#endif

#if NV_CC_GNUC

#if defined(__i386__)

	inline /*volatile*/ uint64 rdtsc()
	{
		uint64 x;
		//__asm__ volatile ("rdtsc" : "=A" (x));
		__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
		return x;
	} 

#elif defined(__x86_64__)

	static __inline__ uint64 rdtsc(void)
	{
		unsigned int hi, lo;
		__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
		return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
	}

#elif defined(__powerpc__)

	static __inline__ uint64 rdtsc(void)
	{
		uint64 result=0;
		unsigned long int upper, lower, tmp;
		__asm__ volatile(
					"0:                  \n"
					"\tmftbu   %0           \n"
					"\tmftb    %1           \n"
					"\tmftbu   %2           \n"
					"\tcmpw    %2,%0        \n"
					"\tbne     0b         \n"
					: "=r"(upper),"=r"(lower),"=r"(tmp)
					);
		result = upper;
		result = result<<32;
		result = result|lower;

		return(result);
	}

#endif

#endif // NV_CC_GNUC


} // nv namespace

#endif // NV_CORE_CPUINFO_H
