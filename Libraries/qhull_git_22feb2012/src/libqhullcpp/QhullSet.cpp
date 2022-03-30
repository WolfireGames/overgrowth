/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullSet.cpp#4 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#//! QhullSet -- Qhull's facet structure, facetT, as a C++ class

#include "QhullError.h"
#include "QhullSet.h"

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#endif

namespace orgQhull {

#//static members

setT QhullSetBase::
s_empty_set;

// Same code for qh_setsize [qset.c] and QhullSetBase::count
int QhullSetBase::count(const setT *set)
{
    int size;
    const setelemT *sizep;

    if (!set)
        return(0);
    sizep= SETsizeaddr_(set);
    if ((size= sizep->i)) {
        size--;
        if (size > set->maxsize) {
            // FIXUP QH11022 How to add additional output to a error? -- qh_setprint(qhmem.ferr, "set: ", set);
            throw QhullError(10032, "QhullSet internal error: current set size %d is greater than maximum size %d\n",
                size, set->maxsize);
        }
    }else
        size= set->maxsize;
    return size;
}


}//namespace orgQhull

