// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/CpuInfo.h>
#include <nvcore/Debug.h>

using namespace nv;

#if NV_OS_WIN32

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

static bool isWow64()
{
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	BOOL bIsWow64 = FALSE;

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			return false;
		}
	}

	return bIsWow64 == TRUE;
}

#endif // NV_OS_WIN32


#if NV_OS_LINUX
#include <string.h>
#include <sched.h>
#endif // NV_OS_LINUX

#if NV_OS_DARWIN
#include <sys/types.h>
#include <sys/sysctl.h>
#endif // NV_OS_DARWIN

// Initialize the data and the local defines, which are designed
// to match the positions in cpuid
uint CpuInfo::m_cpu = ~0x0;
uint CpuInfo::m_procCount = 0;
#define NV_CPUINFO_MMX_MASK  (1<<23)
#define NV_CPUINFO_SSE_MASK  (1<<25)
#define NV_CPUINFO_SSE2_MASK (1<<26)
#define NV_CPUINFO_SSE3_MASK (1)


uint CpuInfo::processorCount()
{
	if (m_procCount == 0) {
#if NV_OS_WIN32
		SYSTEM_INFO sysInfo;

		typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

		if (isWow64())
		{
			GetNativeSystemInfo(&sysInfo);
		}
		else
		{
			GetSystemInfo(&sysInfo);
		}

		uint count = (uint)sysInfo.dwNumberOfProcessors;
		m_procCount = count;

#elif NV_OS_LINUX

		// Code from x264 (July 6 snapshot) cpu.c:271
		uint bit;
		uint np;
		cpu_set_t p_aff;
		memset( &p_aff, 0, sizeof(p_aff) );
		sched_getaffinity( 0, sizeof(p_aff), &p_aff );
		for( np = 0, bit = 0; bit < sizeof(p_aff); bit++ )
			np += (((uint8 *)&p_aff)[bit / 8] >> (bit % 8)) & 1;
		m_procCount = np;

#elif NV_OS_DARWIN

		// Code from x264 (July 6 snapshot) cpu.c:286
		uint numberOfCPUs;
		size_t length = sizeof( numberOfCPUs );
		if( sysctlbyname("hw.ncpu", &numberOfCPUs, &length, NULL, 0) )
		{
			numberOfCPUs = 1;
		}
		m_procCount = numberOfCPUs;

#else
		m_procCount = 1;
#endif
	}
	nvDebugCheck(m_procCount > 0);
	return m_procCount;
}

uint CpuInfo::coreCount()
{
	return 1;
}

bool CpuInfo::hasMMX()
{
	return (cpu() & NV_CPUINFO_MMX_MASK) != 0;
}

bool CpuInfo::hasSSE()
{
	return (cpu() & NV_CPUINFO_SSE_MASK) != 0;
}

bool CpuInfo::hasSSE2()
{
	return (cpu() & NV_CPUINFO_SSE2_MASK) != 0;
}

bool CpuInfo::hasSSE3()
{
	return (cpu() & NV_CPUINFO_SSE3_MASK) != 0;
}

inline int CpuInfo::cpu() {
	if (m_cpu == ~0x0) {
		m_cpu = 0;

#if NV_CC_MSVC
		int CPUInfo[4] = {-1};
		__cpuid(CPUInfo, /*InfoType*/ 1);
		
		if (CPUInfo[2] & NV_CPUINFO_SSE3_MASK) {
			m_cpu |= NV_CPUINFO_SSE3_MASK;
		}
		if (CPUInfo[3] & NV_CPUINFO_MMX_MASK) {
			m_cpu |= NV_CPUINFO_MMX_MASK;
		}
		if (CPUInfo[3] & NV_CPUINFO_SSE_MASK) {
			m_cpu |= NV_CPUINFO_SSE_MASK;
		}
		if (CPUInfo[3] & NV_CPUINFO_SSE2_MASK) {
			m_cpu |= NV_CPUINFO_SSE2_MASK;
		}
#elif NV_CC_GNUC
		// TODO: add the proper inline assembly
#if NV_CPU_X86

#elif NV_CPU_X86_64

#endif	// NV_CPU_X86_64
#endif	// NV_CC_GNUC
	}
	return m_cpu;
}
