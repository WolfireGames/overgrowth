// This code is in the public domain -- castano@gmail.com

#ifndef NV_CORE_TIMER_H
#define NV_CORE_TIMER_H

#include <nvcore/nvcore.h>

#include <time.h> //clock

class NVCORE_CLASS Timer
{
public:
	Timer() {}
	
	void start() { m_start = clock(); }
	int elapsed() const { return (1000 * (clock() - m_start)) / CLOCKS_PER_SEC; }
	
private:
	clock_t m_start;
};

#endif // NV_CORE_TIMER_H
