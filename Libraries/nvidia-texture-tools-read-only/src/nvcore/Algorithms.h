// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_ALGORITHMS_H
#define NV_CORE_ALGORITHMS_H

#include <nvcore/nvcore.h>

namespace nv
{

	/// Return the maximum of two values.
	template <typename T> 
	inline const T & max(const T & a, const T & b)
	{
		//return std::max(a, b);
		if( a < b ) {
			return b; 
		}
		return a;
	}
	
	/// Return the minimum of two values.
	template <typename T> 
	inline const T & min(const T & a, const T & b)
	{
		//return std::min(a, b);
		if( b < a ) {
			return b; 
		}
		return a;
	}

	/// Clamp between two values.
	template <typename T> 
	inline const T & clamp(const T & x, const T & a, const T & b)
	{
		return min(max(x, a), b);
	}
	
	/// Delete all the elements of a container.
	template <typename T>
	void deleteAll(T & container)
	{
		for (typename T::PseudoIndex i = container.start(); !container.isDone(i); container.advance(i))
		{
			delete container[i];
		}
	}
	
	// @@ Should swap be implemented here?



	template <typename T, template <typename T2> class C>
	void sort(C<T> & container)
	{
		introsortLoop(container, 0, container.count());
		insertionSort(container, 0, container.count());
	}

	template <typename T, template <typename T2> class C>
	void sort(C<T> & container, uint begin, uint end)
	{
		if (begin < end)
		{
			introsortLoop(container, begin, end);
			insertionSort(container, begin, end);
		}
	}

	template <typename T, template <typename T2> class C>
	void insertionSort(C<T> & container)
	{
		insertionSort(container, 0, container.count());
	}

	template <typename T, template <typename T2> class C>
	void insertionSort(C<T> & container, uint begin, uint end)
	{
		for (uint i = begin + 1; i != end; ++i)
		{
			T value = container[i];

			uint j = i;
			while (j != begin && container[j-1] > value)
			{
				container[j] = container[j-1];
				--j;
			}
			if (i != j)
			{
				container[j] = value;
			}
		}
	}

	template <typename T, template <typename T2> class C>
    void introsortLoop(C<T> & container, uint begin, uint end)
    {
    	while (end-begin > 16)
    	{
			uint p = partition(container, begin, end, medianof3(container, begin, begin+((end-begin)/2)+1, end-1));
			introsortLoop(container, p, end);
			end = p;
    	}
    }

	template <typename T, template <typename T2> class C>
    uint partition(C<T> & a, uint begin, uint end, const T & x)
    {
    	int i = begin, j = end;
    	while (true)
    	{
    	    while (a[i] < x) ++i;
    	    --j;
    	    while (x < a[j]) --j;
    	    if (i >= j)
    			return i;
    	    swap(a[i], a[j]);
    	    i++;
    	}
    }

	template <typename T, template <typename T2> class C>
    const T & medianof3(C<T> & a, uint lo, uint mid, uint hi)
    {
		if (a[mid] < a[lo])
		{
			if (a[hi] < a[mid])
			{
				return a[mid];
			}
			else
			{
				return (a[hi] < a[lo]) ? a[hi] : a[lo];
			}
		}
		else
		{
			if (a[hi] < a[mid])
			{
				return (a[hi] < a[lo]) ? a[lo] : a[hi];
			}
			else
			{
				return a[mid];
			}
		}
    }


} // nv namespace

#endif // NV_CORE_ALGORITHMS_H
