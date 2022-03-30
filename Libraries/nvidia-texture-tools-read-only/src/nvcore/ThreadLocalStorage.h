// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_THREADLOCALSTORAGE_H
#define NV_CORE_THREADLOCALSTORAGE_H

#include <nvcore/nvcore.h>

// ThreadLocal<Context> context;
//
// context.allocate();
//
//   context = new Context();
//   context->member();
//   context = NULL;
//
// context.free();

#if NV_CC_GNUC

#elif NV_CC_MSVC 

template <class T>
class ThreadLocal
{
public:
	ThreadLocal() : index(0) {}
	~ThreadLocal() { nvCheck(index == 0); }
	
	void allocate()
	{
		index = TlsAlloc();
	}
	void free()
	{
		delete ptr();
		TlsFree(index);
		index = 0;
	}
	bool isValid()
	{
		return index != 0;
	}
	
	void operator=( T * p )
	{
		if (p != ptr())
		{
			delete ptr();
			TlsSetValue(index, p);
		}
	}
	
	T * operator -> () const
	{
		return ptr();
	}
	
	T & operator*() const
	{
		return *ptr();
	}
	
	T * ptr() const { 
		return static_cast<T *>(TlsGetValue(index));
	}
	
	DWORD index;
};


#endif // NV_CC_MSVC

#endif // NV_CORE_THREADLOCALSTORAGE_H
