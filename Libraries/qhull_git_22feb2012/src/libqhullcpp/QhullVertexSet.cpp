/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullVertexSet.cpp#5 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#//! QhullVertexSet -- Qhull's linked Vertexs, as a C++ class

#include "QhullVertex.h"
#include "QhullVertexSet.h"
#include "QhullPoint.h"
#include "QhullRidge.h"
#include "QhullVertex.h"

using std::string;
using std::vector;

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#pragma warning( disable : 4611)  /* interaction between '_setjmp' and C++ object destruction is non-portable */
                                    /* setjmp should not be implemented with 'catch' */
#endif

namespace orgQhull {

QhullVertexSet::
QhullVertexSet(int qhRunId, facetT *facetlist, setT *facetset, bool allfacets)
: QhullSet<QhullVertex>(0)
, qhsettemp_qhull(0)
, qhsettemp_defined(false)
{
    UsingLibQhull q(qhRunId);
    int exitCode = setjmp(qh errexit);
    if(!exitCode){ // no object creation -- destructors skipped on longjmp()
        setT *vertices= qh_facetvertices(facetlist, facetset, allfacets);
        defineAs(vertices);
        qhsettemp_qhull= s_qhull_output;
        qhsettemp_defined= true;
    }
    q.maybeThrowQhullMessage(exitCode);
}//QhullVertexSet facetlist facetset

void QhullVertexSet::
freeQhSetTemp()
{
    if(qhsettemp_defined){
        UsingLibQhull q(qhsettemp_qhull, QhullError::NOthrow);
        if(q.defined()){
            int exitCode = setjmp(qh errexit);
            if(!exitCode){ // no object creation -- destructors skipped on longjmp()
                qh_settempfree(referenceSetT()); // errors if not top of tempstack or if qhmem corrupted
            }
            q.maybeThrowQhullMessage(exitCode, QhullError::NOthrow);
        }
    }
}//freeQhSetTemp

QhullVertexSet::
~QhullVertexSet()
{
    freeQhSetTemp();
}//~QhullVertexSet

}//namespace orgQhull

#//Global functions

using std::endl;
using std::ostream;
using orgQhull::QhullPoint;
using orgQhull::QhullVertex;
using orgQhull::QhullVertexSet;
using orgQhull::QhullVertexSetIterator;
using orgQhull::UsingLibQhull;

//! Print Vertex identifiers to stream.  Space prefix.  From qh_printVertexheader [io.c]
ostream &
operator<<(ostream &os, const QhullVertexSet::PrintIdentifiers &pr)
{
    if(pr.print_message && *pr.print_message){
        os << pr.print_message;
    }
    for(QhullVertexSet::const_iterator i=pr.Vertex_set->begin(); i!=pr.Vertex_set->end(); ++i){
        const QhullVertex v= *i;
        os << " v" << v.id();
    }
    os << endl;
    return os;
}//<<QhullVertexSet::PrintIdentifiers

//! Duplicate of printvertices [io.c]
//! If pr.run_id==UsingLibQhull::NOqhRunId, no access to qh [needed for QhullPoint]
ostream &
operator<<(ostream &os, const QhullVertexSet::PrintVertexSet &pr){

    os << pr.print_message;
    const QhullVertexSet *vs= pr.Vertex_set;
    QhullVertexSetIterator i= *vs;
    while(i.hasNext()){
        const QhullVertex v= i.next();
        const QhullPoint p= v.point();
        os << " p" << p.id(pr.run_id) << "(v" << v.id() << ")";
    }
    os << endl;

    return os;
}//<< PrintVertexSet


