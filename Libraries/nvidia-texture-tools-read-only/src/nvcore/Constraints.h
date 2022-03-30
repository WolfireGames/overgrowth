// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_ALGORITHMS_H
#define NV_CORE_ALGORITHMS_H

#include <nvcore/nvcore.h>

namespace nv
{
	// Cool constraints from "Imperfect C++"

	// must_be_pod
	template <typename T>
	struct must_be_pod
	{
		static void constraints()
		{
			union { T T_is_not_POD_type; };
		}
	};

	// must_be_pod_or_void
	template <typename T>
	struct must_be_pod_or_void
	{
		static void constraints()
		{
			union { T T_is_not_POD_type; };
		}
	};
	template <> struct must_be_pod_or_void<void> {};

	// size_of
	template <typename T>
	struct size_of
	{
		enum { value = sizeof(T) };
	};
	template <> 
	struct size_of<void>
	{
		enum { value = 0 };
	};
	
	// must_be_same_size
	template <typename T1, typename T2>
	struct must_be_same_size
	{
		static void constraints()
		{
			const int T1_not_same_size_as_T2 = size_of<T1>::value == size_of<T2>::value;
			int i[T1_not_same_size_as_T2];
		}
	};


} // nv namespace

#endif // NV_CORE_ALGORITHMS_H
