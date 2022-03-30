/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullQh.cpp#3 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#//! QhullQh -- Qhull's global data structure, qhT, as a C++ class



#include "QhullError.h"
#include "QhullQh.h"
#include "QhullStat.h"

#include <sstream>
#include <iostream>

using std::cerr;
using std::string;
using std::vector;
using std::ostream;

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#pragma warning( disable : 4611)  // interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning( disable : 4996)  // function was declared deprecated(strcpy, localtime, etc.)
#endif

namespace orgQhull {

#//Global variables

#//Constructor, destructor, etc.

//! If qh_QHpointer==0, invoke with placement new on qh_qh;
//! Sets qh_qh and qh_qhstat.  Need to reset before UsingLibQhull.
//! Derived from qh_new_qhull[user.c]
QhullQh::
QhullQh()
{
    static boolT firstcall = True;

    if(firstcall){
        if(qhmem.BUFinit!=0){
            throw QhullError(10017, "Qhull error: qhmem already initialized by another class.");
        }
        qh_meminit(NULL);
        firstcall= False;
    }
    // QhullQh() and UsingLibQhull() are the same
#if qh_QHpointer
    if(qh_qh){
        if(qh old_qhstat){
            throw QhullError(10041, "Qhull internal error: qh_qh.old_qhstat defined (%x) but qh_qh is active.  qh_qh not restored correctly.", 0, 0, 0.0, qh old_qhstat);
        }
        qh old_qhstat= qh_qhstat;
        qh old_tempstack= qhmem.tempstack;
        qh_qhstat= 0;
        qhmem.tempstack= 0;
    }
    qh_qh= static_cast<qhT*>(this);
#else
    if(strncmp(qh qhull, "qhull", 5) == 0){
        throw QhullError(10022, "Qhull error: Qhull already initialized as run %d", qh run_id);
    }
#endif
    // NOerrors -- Does not call qh_errexit()
    qh_initstatistics();
    // NOerrors -- Does not call qh_errexit()
    qh_initqhull_start2(NULL, NULL, qh_FILEstderr);
}//QhullQh

//! UsingLibQhull must be declared along with QhullQh
QhullQh::
~QhullQh()
{
#if qh_QHpointer
    if(!qh_qh){
        QhullError e(10042, "Qhull internal error: qh_qh undefined.  Was ~QhullQh() invoked independent of UsingLibQhull?", qh run_id, 0, 0, qh_qh);
        e.logError();
    }else if(!qh_qhstat){
        QhullError e(10043, "Qhull internal error: qh_qhstat null.  Is another thread running?");
        e.logError();
    }else if(qh_qh!=this){
        QhullError e(10044, "Qhull error: ~QhullQh() invoked independent of UsingLibQhull. qh_qh %x (runId %d) vs. QhullQh.runId %d.", qh run_id, run_id, 0.0, qh_qh);
        e.logError();
    }else{
        qh_freeqhull2(qh_ALL); // sets qh.NOerrexit.  Clears struct *qh_qh including run_id, but not qh_qh itself
    }
#else
    if(&qh_qh!=this){
        QhullError e(10045, "Qhull error: ~QhullQh() invoked independent of UsingLibQhull. qh_qh %x (runId %d) vs. QhullQh.runId %d.", qh run_id, run_id, 0.0, qh_qh);
        e.logError();
    }else{
        qh_freeqhull2(qh_ALL); // sets qh.NOerrexit.  Clears struct *qh_qh including run_id, but not qh_qh itself
    }
#endif
}//~QhullQh

#//Parallel Access

}//namespace orgQhull

