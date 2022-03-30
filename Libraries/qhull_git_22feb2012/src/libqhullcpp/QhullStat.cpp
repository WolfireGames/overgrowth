/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullStat.cpp#3 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#//! QhullStat -- Qhull's global data structure, statT, as a C++ class


#include "QhullError.h"
#include "QhullStat.h"

#include <sstream>
#include <iostream>

using std::cerr;
using std::string;
using std::vector;
using std::ostream;

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#endif

namespace orgQhull {

#//Constructor, destructor, etc.

//! If qh_QHpointer==0, invoke with placement new on qh_stat;
QhullStat::
QhullStat()
{
}//QhullStat

QhullStat::
~QhullStat()
{
}//~QhullStat

}//namespace orgQhull

