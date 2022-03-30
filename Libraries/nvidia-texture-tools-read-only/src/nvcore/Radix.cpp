///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains source code from the article "Radix Sort Revisited".
 *	\file		Radix.cpp
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// References:
// http://www.codercorner.com/RadixSortRevisited.htm
// http://www.stereopsis.com/radix.html

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Revisited Radix Sort.
 *	This is my new radix routine:
 *  - it uses indices and doesn't recopy the values anymore, hence wasting less ram
 *  - it creates all the histograms in one run instead of four
 *  - it sorts words faster than dwords and bytes faster than words
 *  - it correctly sorts negative floating-point values by patching the offsets
 *  - it automatically takes advantage of temporal coherence
 *  - multiple keys support is a side effect of temporal coherence
 *  - it may be worth recoding in asm... (mainly to use FCOMI, FCMOV, etc) [it's probably memory-bound anyway]
 *
 *	History:
 *	- 08.15.98: very first version
 *	- 04.04.00: recoded for the radix article
 *	- 12.xx.00: code lifting
 *	- 09.18.01: faster CHECK_PASS_VALIDITY thanks to Mark D. Shattuck (who provided other tips, not included here)
 *	- 10.11.01: added local ram support
 *	- 01.20.02: bugfix! In very particular cases the last pass was skipped in the float code-path, leading to incorrect sorting......
 *	- 01.02.02:	- "mIndices" renamed => "mRanks". That's a rank sorter after all.
 *				- ranks are not "reset" anymore, but implicit on first calls
 *	- 07.05.02:	offsets rewritten with one less indirection.
 *	- 11.03.02:	"bool" replaced with RadixHint enum
 *	- 07.15.04:	stack-based radix added
 *				- we want to use the radix sort but without making it static, and without allocating anything.
 *				- we internally allocate two arrays of ranks. Each of them has N uint32s to sort N values.
 *				- 1Mb/2/sizeof(uint32) = 131072 values max, at the same time.
 *	- 09.22.04:	- adapted to MacOS by Chris Lamb
 *	- 01.12.06:	- added optimizations suggested by Kyle Hubert
 *	- 04.06.08:	- Fix bug negative zero sorting bug by Ignacio Castaño
 *
 *	\class		RadixSort
 *	\author		Pierre Terdiman
 *	\version	1.5
 *	\date		August, 15, 1998
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Header

#include <nvcore/Radix.h>

#include <string.h> // memset

//using namespace IceCore;

#define INVALIDATE_RANKS	mCurrentSize|=0x80000000
#define VALIDATE_RANKS		mCurrentSize&=0x7fffffff
#define CURRENT_SIZE		(mCurrentSize&0x7fffffff)
#define INVALID_RANKS		(mCurrentSize&0x80000000)

#if NV_BIG_ENDIAN
	#define H0_OFFSET	768
	#define H1_OFFSET	512
	#define H2_OFFSET	256
	#define H3_OFFSET	0
	#define BYTES_INC	(3-j)
#else 
	#define H0_OFFSET	0
	#define H1_OFFSET	256
	#define H2_OFFSET	512
	#define H3_OFFSET	768
	#define BYTES_INC	j
#endif

#define CREATE_HISTOGRAMS(type, buffer)														\
	/* Clear counters/histograms */															\
	memset(mHistogram, 0, 256*4*sizeof(uint32));											\
																							\
	/* Prepare to count */																	\
	const uint8* p = (const uint8*)input;													\
	const uint8* pe = &p[nb*4];																\
	uint32* h0= &mHistogram[H0_OFFSET];	/* Histogram for first pass (LSB)	*/				\
	uint32* h1= &mHistogram[H1_OFFSET];	/* Histogram for second pass		*/				\
	uint32* h2= &mHistogram[H2_OFFSET];	/* Histogram for third pass			*/				\
	uint32* h3= &mHistogram[H3_OFFSET];	/* Histogram for last pass (MSB)	*/				\
																							\
	bool AlreadySorted = true;	/* Optimism... */											\
																							\
	if(INVALID_RANKS)																		\
	{																						\
		/* Prepare for temporal coherence */												\
		type* Running = (type*)buffer;														\
		type PrevVal = *Running;															\
																							\
		while(p!=pe)																		\
		{																					\
			/* Read input buffer in previous sorted order */								\
			type Val = *Running++;															\
			/* Check whether already sorted or not */										\
			if(Val<PrevVal)	{ AlreadySorted = false; break; } /* Early out */				\
			/* Update for next iteration */													\
			PrevVal = Val;																	\
																							\
			/* Create histograms */															\
			h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;									\
		}																					\
																							\
		/* If all input values are already sorted, we just have to return and leave the */	\
		/* previous list unchanged. That way the routine may take advantage of temporal */	\
		/* coherence, for example when used to sort transparent faces.					*/	\
		if(AlreadySorted)																	\
		{																					\
			mNbHits++;																		\
			for(uint32 i=0;i<nb;i++)	mRanks[i] = i;										\
			return *this;																	\
		}																					\
	}																						\
	else																					\
	{																						\
		/* Prepare for temporal coherence */												\
		const uint32* Indices = mRanks;														\
		type PrevVal = (type)buffer[*Indices];												\
																							\
		while(p!=pe)																		\
		{																					\
			/* Read input buffer in previous sorted order */								\
			type Val = (type)buffer[*Indices++];											\
			/* Check whether already sorted or not */										\
			if(Val<PrevVal)	{ AlreadySorted = false; break; } /* Early out */				\
			/* Update for next iteration */													\
			PrevVal = Val;																	\
																							\
			/* Create histograms */															\
			h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;									\
		}																					\
																							\
		/* If all input values are already sorted, we just have to return and leave the */	\
		/* previous list unchanged. That way the routine may take advantage of temporal */	\
		/* coherence, for example when used to sort transparent faces.					*/	\
		if(AlreadySorted)	{ mNbHits++; return *this;	}									\
	}																						\
																							\
	/* Else there has been an early out and we must finish computing the histograms */		\
	while(p!=pe)																			\
	{																						\
		/* Create histograms without the previous overhead */								\
		h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;										\
	}

#define CHECK_PASS_VALIDITY(pass)															\
	/* Shortcut to current counters */														\
	const uint32* CurCount = &mHistogram[pass<<8];											\
																							\
	/* Reset flag. The sorting pass is supposed to be performed. (default) */				\
	bool PerformPass = true;																\
																							\
	/* Check pass validity */																\
																							\
	/* If all values have the same byte, sorting is useless. */								\
	/* It may happen when sorting bytes or words instead of dwords. */						\
	/* This routine actually sorts words faster than dwords, and bytes */					\
	/* faster than words. Standard running time (O(4*n))is reduced to O(2*n) */				\
	/* for words and O(n) for bytes. Running time for floats depends on actual values... */	\
																							\
	/* Get first byte */																	\
	uint8 UniqueVal = *(((uint8*)input)+pass);												\
																							\
	/* Check that byte's counter */															\
	if(CurCount[UniqueVal]==nb)	PerformPass=false;

using namespace nv;

/// Constructor.
RadixSort::RadixSort() : mRanks(NULL), mRanks2(NULL), mCurrentSize(0), mTotalCalls(0), mNbHits(0), mDeleteRanks(true)
{
	// Initialize indices
	INVALIDATE_RANKS;
}

/// Destructor.
RadixSort::~RadixSort()
{
	// Release everything
	if(mDeleteRanks)
	{
		delete [] mRanks2;
		delete [] mRanks;
	}
}

/// Resizes the inner lists.
/// \param		nb				[in] new size (number of dwords)
/// \return		true if success
bool RadixSort::resize(uint32 nb)
{
	if(mDeleteRanks)
	{
		// Free previously used ram
		delete [] mRanks2;
		delete [] mRanks;

		// Get some fresh one
		mRanks	= new uint32[nb];
		mRanks2	= new uint32[nb];
	}
	return true;

}

inline void RadixSort::checkResize(uint32 nb)
{
	uint32 CurSize = CURRENT_SIZE;
	if(nb!=CurSize)
	{
		if(nb>CurSize) resize(nb);
		mCurrentSize = nb;
		INVALIDATE_RANKS;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Main sort routine.
 *	This one is for integer values. After the call, mIndices contains a list of indices in sorted order, i.e. in the order you may process your data.
 *	\param		input			[in] a list of integer values to sort
 *	\param		nb				[in] number of values to sort
 *	\param		signedvalues	[in] true to handle negative values, false if you know your input buffer only contains positive values
 *	\return		Self-Reference
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RadixSort& RadixSort::sort(const uint32* input, uint32 nb, bool signedValues/*=true*/)
{
	// Checkings
	if(!input || !nb || nb&0x80000000)	return *this;

	// Stats
	mTotalCalls++;

	// Resize lists if needed
	checkResize(nb);

	// Allocate histograms & offsets on the stack
	uint32 mHistogram[256*4];
	uint32* mLink[256];

	// Create histograms (counters). Counters for all passes are created in one run.
	// Pros:	read input buffer once instead of four times
	// Cons:	mHistogram is 4Kb instead of 1Kb
	// We must take care of signed/unsigned values for temporal coherence.... I just
	// have 2 code paths even if just a single opcode changes. Self-modifying code, someone?
	if(!signedValues)	{ CREATE_HISTOGRAMS(uint32, input);	}
	else				{ CREATE_HISTOGRAMS(int32, input);	}

	// Radix sort, j is the pass number (0=LSB, 3=MSB)
	for(uint32 j=0;j<4;j++)
	{
		CHECK_PASS_VALIDITY(j);

		// Sometimes the fourth (negative) pass is skipped because all numbers are negative and the MSB is 0xFF (for example). This is
		// not a problem, numbers are correctly sorted anyway.
		if(PerformPass)
		{
			// Should we care about negative values?
			if(j!=3 || !signedValues)
			{
				// Here we deal with positive values only

				// Create offsets
				mLink[0] = mRanks2;
				for(uint32 i=1;i<256;i++)		mLink[i] = mLink[i-1] + CurCount[i-1];
			}
			else
			{
				// This is a special case to correctly handle negative integers. They're sorted in the right order but at the wrong place.
				mLink[128] = mRanks2;
				for(uint32 i=129;i<256;i++)	mLink[i] = mLink[i-1] + CurCount[i-1];

				mLink[0] = mLink[255] + CurCount[255];
				for(uint32 i=1;i<128;i++)	mLink[i] = mLink[i-1] + CurCount[i-1];
			}

			// Perform Radix Sort
			const uint8* InputBytes	= (const uint8*)input;
			InputBytes += BYTES_INC;
			if(INVALID_RANKS)
			{
				for(uint32 i=0;i<nb;i++)	*mLink[InputBytes[i<<2]]++ = i;
				VALIDATE_RANKS;
			}
			else
			{
				const uint32* Indices		= mRanks;
				const uint32* IndicesEnd	= &mRanks[nb];
				while(Indices!=IndicesEnd)
				{
					uint32 id = *Indices++;
					*mLink[InputBytes[id<<2]]++ = id;
				}
			}

			// Swap pointers for next pass. Valid indices - the most recent ones - are in mRanks after the swap.
			uint32* Tmp = mRanks;
			mRanks = mRanks2;
			mRanks2 = Tmp;
		}
	}
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Main sort routine.
 *	This one is for floating-point values. After the call, mIndices contains a list of indices in sorted order, i.e. in the order you may process your data.
 *	\param		input			[in] a list of floating-point values to sort
 *	\param		nb				[in] number of values to sort
 *	\return		Self-Reference
 *	\warning	only sorts IEEE floating-point values
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RadixSort& RadixSort::sort(const float* input2, uint32 nb)
{
	// Checkings
	if(!input2 || !nb || nb&0x80000000)	return *this;

	// Stats
	mTotalCalls++;

	const uint32* input = (const uint32*)input2;

	// Resize lists if needed
	checkResize(nb);

	// Allocate histograms & offsets on the stack
	uint32 mHistogram[256*4];
	uint32* mLink[256];

	// Create histograms (counters). Counters for all passes are created in one run.
	// Pros:	read input buffer once instead of four times
	// Cons:	mHistogram is 4Kb instead of 1Kb
	// Floating-point values are always supposed to be signed values, so there's only one code path there.
	// Please note the floating point comparison needed for temporal coherence! Although the resulting asm code
	// is dreadful, this is surprisingly not such a performance hit - well, I suppose that's a big one on first
	// generation Pentiums....We can't make comparison on integer representations because, as Chris said, it just
	// wouldn't work with mixed positive/negative values....
	{ CREATE_HISTOGRAMS(float, input2); }

	// Radix sort, j is the pass number (0=LSB, 3=MSB)
	for(uint32 j=0;j<4;j++)
	{
		// Should we care about negative values?
		if(j!=3)
		{
			// Here we deal with positive values only
			CHECK_PASS_VALIDITY(j);

			if(PerformPass)
			{
				// Create offsets
				mLink[0] = mRanks2;
				for(uint32 i=1;i<256;i++)		mLink[i] = mLink[i-1] + CurCount[i-1];

				// Perform Radix Sort
				const uint8* InputBytes = (const uint8*)input;
				InputBytes += BYTES_INC;
				if(INVALID_RANKS)
				{
					for(uint32 i=0;i<nb;i++)	*mLink[InputBytes[i<<2]]++ = i;
					VALIDATE_RANKS;
				}
				else
				{
					const uint32* Indices		= mRanks;
					const uint32* IndicesEnd	= &mRanks[nb];
					while(Indices!=IndicesEnd)
					{
						uint32 id = *Indices++;
						*mLink[InputBytes[id<<2]]++ = id;
					}
				}

				// Swap pointers for next pass. Valid indices - the most recent ones - are in mRanks after the swap.
				uint32* Tmp = mRanks;
				mRanks = mRanks2;
				mRanks2 = Tmp;
			}
		}
		else
		{
			// This is a special case to correctly handle negative values
			CHECK_PASS_VALIDITY(j);

			if(PerformPass)
			{
				mLink[255] = mRanks2 + CurCount[255];
				for(uint32 i = 254; i > 126; i--) mLink[i] = mLink[i+1] + CurCount[i];
				mLink[0] = mLink[127] + CurCount[127];
				for(uint32 i = 1; i < 127; i++) mLink[i] = mLink[i-1] + CurCount[i-1];

				// Perform Radix Sort
				if(INVALID_RANKS)
				{
					for(uint32 i=0;i<nb;i++)
					{
						uint32 Radix = input[i]>>24;							// Radix byte, same as above. AND is useless here (uint32).
						// ### cmp to be killed. Not good. Later.
						if(Radix<128)		*mLink[Radix]++ = i;		// Number is positive, same as above
						else				*(--mLink[Radix]) = i;		// Number is negative, flip the sorting order
					}
					VALIDATE_RANKS;
				}
				else
				{
					for(uint32 i=0;i<nb;i++)
					{
						uint32 Radix = input[mRanks[i]]>>24;							// Radix byte, same as above. AND is useless here (uint32).
						// ### cmp to be killed. Not good. Later.
						if(Radix<128)		*mLink[Radix]++ = mRanks[i];		// Number is positive, same as above
						else				*(--mLink[Radix]) = mRanks[i];		// Number is negative, flip the sorting order
					}
				}
				// Swap pointers for next pass. Valid indices - the most recent ones - are in mRanks after the swap.
				uint32* Tmp = mRanks;
				mRanks = mRanks2;
				mRanks2 = Tmp;
			}
			else
			{
				// The pass is useless, yet we still have to reverse the order of current list if all values are negative.
				if(UniqueVal>=128)
				{
					if(INVALID_RANKS)
					{
						// ###Possible?
						for(uint32 i=0;i<nb;i++)	mRanks2[i] = nb-i-1;
						VALIDATE_RANKS;
					}
					else
					{
						for(uint32 i=0;i<nb;i++)	mRanks2[i] = mRanks[nb-i-1];
					}

					// Swap pointers for next pass. Valid indices - the most recent ones - are in mRanks after the swap.
					uint32* Tmp = mRanks;
					mRanks = mRanks2;
					mRanks2 = Tmp;
				}
			}
		}
	}
	return *this;
}


bool RadixSort::setRankBuffers(uint32* ranks0, uint32* ranks1)
{
	if(!ranks0 || !ranks1)	return false;

	mRanks			= ranks0;
	mRanks2			= ranks1;
	mDeleteRanks	= false;

	return true;
}

RadixSort & RadixSort::sort(const Array<int> & input)
{
	return sort((const uint32 *)input.buffer(), input.count(), true);
}

RadixSort & RadixSort::sort(const Array<uint> & input)
{
	return sort(input.buffer(), input.count(), false);
}

RadixSort &	RadixSort::sort(const Array<float> & input)
{
	return sort(input.buffer(), input.count());
}
