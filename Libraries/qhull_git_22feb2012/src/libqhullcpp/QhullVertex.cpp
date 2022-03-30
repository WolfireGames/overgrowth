/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullVertex.cpp#3 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#//! QhullVertex -- Qhull's vertex structure, vertexT, as a C++ class

#include "UsingLibQhull.h"
#include "QhullPoint.h"
#include "QhullFacetSet.h"
#include "QhullVertex.h"
#include "QhullVertexSet.h"
#include "QhullFacet.h"

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#pragma warning( disable : 4611)  // interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning( disable : 4996)  // function was declared deprecated(strcpy, localtime, etc.)
#endif

namespace orgQhull {

#//class statics
vertexT QhullVertex::
s_empty_vertex= {0,0,0,0,0,
                 0,0,0,0,0,
                 0,0};

#//ForEach

//! Return neighboring facets for a vertex
//! If neither merging nor Voronoi diagram, requires Qhull::defineVertexNeighborFacets() beforehand.
QhullFacetSet QhullVertex::
neighborFacets() const
{
    if(!neighborFacetsDefined()){
        throw QhullError(10034, "Qhull error: neighboring facets of vertex %d not defined.  Please call Qhull::defineVertexNeighborFacets() beforehand.", id());
    }
    return QhullFacetSet(qh_vertex->neighbors);
}//neighborFacets

}//namespace orgQhull

#//Global functions

using std::endl;
using std::ostream;
using std::string;
using std::vector;
using orgQhull::QhullPoint;
using orgQhull::QhullFacet;
using orgQhull::QhullFacetSet;
using orgQhull::QhullFacetSetIterator;
using orgQhull::QhullVertex;
using orgQhull::UsingLibQhull;

//! Duplicate of qh_printvertex [io.c]
ostream &
operator<<(ostream &os, const QhullVertex::PrintVertex &pr)
{
    QhullVertex v= *pr.vertex;
    QhullPoint p= v.point();
    os << "- p" << p.id(pr.run_id) << " (v" << v.id() << "): ";
    const realT *c= p.coordinates();
    for(int k= p.dimension(); k--; ){
        os << " " << *c++; // FIXUP QH11010 %5.2g
    }
    if(v.getVertexT()->deleted){
        os << " deleted";
    }
    if(v.getVertexT()->delridge){
        os << " ridgedeleted";
    }
    os << endl;
    if(v.neighborFacetsDefined()){
        QhullFacetSetIterator i= v.neighborFacets();
        if(i.hasNext()){
            os << " neighborFacets:";
            int count= 0;
            while(i.hasNext()){
                if(++count % 100 == 0){
                    os << endl << "     ";
                }
                QhullFacet f= i.next();
                os << " f" << f.id();
            }
            os << endl;
        }
    }
    return os;
}//<< PrintVertex

