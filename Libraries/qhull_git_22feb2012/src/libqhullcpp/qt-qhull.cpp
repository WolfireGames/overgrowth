/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/qt-qhull.cpp#4 $$Change: 1490 $
** $DateTime: 2012/02/19 20:27:01 $$Author: bbarber $
**
****************************************************************************/

#include <QList>
#include "RoadTest.h"

#ifndef QHULL_USES_QT
#define QHULL_USES_QT 1
#endif

#include "Coordinates.h"
#include "QhullFacetList.h"
#include "QhullFacetSet.h"
#include "QhullHyperplane.h"
#include "QhullPoint.h"
#include "QhullPoints.h"
#include "QhullPointSet.h"
#include "QhullVertex.h"
#include "QhullVertexSet.h"

namespace orgQhull {

#//Conversions

QList<coordT> Coordinates::
toQList() const
{
    CoordinatesIterator i(*this);
    QList<coordT> cs;
    while(i.hasNext()){
        cs.append(i.next());
    }
    return cs;
}//toQList

QList<QhullFacet> QhullFacetList::
toQList() const
{
    QhullLinkedListIterator<QhullFacet> i(*this);
    QList<QhullFacet> vs;
    while(i.hasNext()){
        QhullFacet f= i.next();
        if(isSelectAll() || f.isGood()){
            vs.append(f);
        }
    }
    return vs;
}//toQList

//! Same as PrintVertices
QList<QhullVertex> QhullFacetList::
vertices_toQList(int qhRunId) const
{
    QList<QhullVertex> vs;
    QhullVertexSet qvs(qhRunId, first().getFacetT(), NULL, isSelectAll());
    for(QhullVertexSet::iterator i=qvs.begin(); i!=qvs.end(); ++i){
        vs.push_back(*i);
    }
    return vs;
}//vertices_toQList

QList<QhullFacet> QhullFacetSet::
toQList() const
{
    QhullSetIterator<QhullFacet> i(*this);
    QList<QhullFacet> vs;
    while(i.hasNext()){
        QhullFacet f= i.next();
        if(isSelectAll() || f.isGood()){
            vs.append(f);
        }
    }
    return vs;
}//toQList

#ifdef QHULL_USES_QT
QList<coordT> QhullHyperplane::
toQList() const
{
    QhullHyperplaneIterator i(*this);
    QList<coordT> fs;
    while(i.hasNext()){
        fs.append(i.next());
    }
    fs.append(hyperplane_offset);
    return fs;
}//toQList
#endif //QHULL_USES_QT

QList<coordT> QhullPoint::
toQList() const
{
    QhullPointIterator i(*this);
    QList<coordT> vs;
    while(i.hasNext()){
        vs.append(i.next());
    }
    return vs;
}//toQList

QList<QhullPoint> QhullPoints::
toQList() const
{
    QhullPointsIterator i(*this);
    QList<QhullPoint> vs;
    while(i.hasNext()){
        vs.append(i.next());
    }
    return vs;
}//toQList

QList<QhullPoint> QhullPointSet::
toQList() const
{
    QhullPointSetIterator i(*this);
    QList<QhullPoint> vs;
    while(i.hasNext()){
        vs.append(i.next());
    }
    return vs;
}//toQList

}//orgQhull

