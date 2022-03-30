/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullFacetList.cpp#3 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#//! QhullFacetList -- Qhull's linked facets, as a C++ class

#include "QhullFacet.h"
#include "QhullFacetList.h"
#include "QhullPoint.h"
#include "QhullRidge.h"
#include "QhullVertex.h"

using std::string;
using std::vector;

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#pragma warning( disable : 4611)  // interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning( disable : 4996)  // function was declared deprecated(strcpy, localtime, etc.)
#endif

namespace orgQhull {

#//Conversion

// See qt_qhull.cpp for QList conversions

#ifndef QHULL_NO_STL
std::vector<QhullFacet> QhullFacetList::
toStdVector() const
{
    QhullLinkedListIterator<QhullFacet> i(*this);
    std::vector<QhullFacet> vs;
    while(i.hasNext()){
        QhullFacet f= i.next();
        if(isSelectAll() || f.isGood()){
            vs.push_back(f);
        }
    }
    return vs;
}//toStdVector
#endif //QHULL_NO_STL

#ifndef QHULL_NO_STL
//! Same as PrintVertices
std::vector<QhullVertex> QhullFacetList::
vertices_toStdVector(int qhRunId) const
{
    std::vector<QhullVertex> vs;
    QhullVertexSet qvs(qhRunId, first().getFacetT(), NULL, isSelectAll());

    for(QhullVertexSet::iterator i=qvs.begin(); i!=qvs.end(); ++i){
        vs.push_back(*i);
    }
    return vs;
}//vertices_toStdVector
#endif //QHULL_NO_STL

#//Read-only

bool QhullFacetList::
contains(const QhullFacet &facet) const
{
    if(isSelectAll()){
        return QhullLinkedList<QhullFacet>::contains(facet);
    }
    for(QhullFacetList::const_iterator i=begin(); i != end(); ++i){
        QhullFacet f= *i;
        if(f==facet && f.isGood()){
            return true;
        }
    }
    return false;
}//contains

int QhullFacetList::
count() const
{
    if(isSelectAll()){
        return QhullLinkedList<QhullFacet>::count();
    }
    int counter= 0;
    for(QhullFacetList::const_iterator i=begin(); i != end(); ++i){
        if((*i).isGood()){
            counter++;
        }
    }
    return counter;
}//count

int QhullFacetList::
count(const QhullFacet &facet) const
{
    if(isSelectAll()){
        return QhullLinkedList<QhullFacet>::count(facet);
    }
    int counter= 0;
    for(QhullFacetList::const_iterator i=begin(); i != end(); ++i){
        QhullFacet f= *i;
        if(f==facet && f.isGood()){
            counter++;
        }
    }
    return counter;
}//count

}//namespace orgQhull

#//Global functions

using std::endl;
using std::ostream;
using orgQhull::QhullFacet;
using orgQhull::QhullFacetList;
using orgQhull::QhullVertex;
using orgQhull::QhullVertexSet;
using orgQhull::UsingLibQhull;

ostream &
operator<<(ostream &os, const QhullFacetList::PrintFacetList &pr)
{
    QhullFacetList fs= *pr.facet_list;
    os << "Vertices for " << fs.count() << " facets" << endl;
    os << fs.printVertices(pr.run_id);
    os << fs.printFacets(pr.run_id);
    return os;
}//operator<<

//! Print facet list to stream.  From qh_printafacet [io.c]
ostream &
operator<<(ostream &os, const QhullFacetList::PrintFacets &pr)
{
    for(QhullFacetList::const_iterator i= pr.facet_list->begin(); i != pr.facet_list->end(); ++i){
        QhullFacet f= *i;
        if(pr.facet_list->isSelectAll() || f.isGood()){
            os << f.print(pr.run_id);
        }
    }
    return os;
}//printFacets

//! Print vertices of good faces in facet list to stream.  From qh_printvertexlist [io.c]
//! Same as vertices_toStdVector
ostream &
operator<<(ostream &os, const QhullFacetList::PrintVertices &pr)
{
    QhullVertexSet vs(pr.run_id, pr.facet_list->first().getFacetT(), NULL, pr.facet_list->isSelectAll());
    for(QhullVertexSet::iterator i=vs.begin(); i!=vs.end(); ++i){
        QhullVertex v= *i;
        os << v.print(pr.run_id);
    }
    return os;
}//printVertices

std::ostream &
operator<<(ostream &os, const QhullFacetList &fs)
{
    os << fs.printFacets(UsingLibQhull::NOqhRunId);
    return os;
}//QhullFacetList

