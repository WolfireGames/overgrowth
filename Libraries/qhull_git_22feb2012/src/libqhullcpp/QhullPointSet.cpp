/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullPointSet.cpp#5 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#include "QhullPointSet.h"

#include <iostream>
#include <algorithm>

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#endif

namespace orgQhull {

#// Conversion

// See qt-qhull.cpp for QList conversion

#ifndef QHULL_NO_STL
std::vector<QhullPoint> QhullPointSet::
toStdVector() const
{
    QhullPointSetIterator i(*this);
    std::vector<QhullPoint> vs;
    while(i.hasNext()){
        vs.push_back(i.next());
    }
    return vs;
}//toStdVector
#endif //QHULL_NO_STL

#//Element-access
//! Derived from QhullSet::value
QhullPoint QhullPointSet::
value(int idx) const
{
    // Avoid call to qh_setsize() and assert in elementPointer()
    //const T *n= reinterpret_cast<const T *>(&SETelem_(getSetT(), idx));
    void **n= reinterpret_cast<void **>(&SETelem_(getSetT(), idx));
    coordT **n2= reinterpret_cast<coordT **>(n);
    if(idx>=0 && n<endPointer()){
        return QhullPoint(dimension(), *n2);
    }else{
        return QhullPoint();
    }
}//value

//! Non-const since copy is an alias
//! Derived from QhullSet::value
QhullPoint QhullPointSet::
value(int idx, QhullPoint &defaultValue) const
{
    // Avoid call to qh_setsize() and assert in elementPointer()
    void **n= reinterpret_cast<void **>(&SETelem_(getSetT(), idx));
    coordT **n2= reinterpret_cast<coordT **>(n);
    if(idx>=0 && n<endPointer()){
        return QhullPoint(dimension(), *n2);
    }else{
        return defaultValue;
    }
}//value

#//Read-only

bool QhullPointSet::
operator==(const QhullPointSet &o) const
{
    if(dimension()!=o.dimension() || count()!=o.count()){
        return false;
    }
    QhullPointSetIterator i(*this);
    QhullPointSetIterator j(o);
    while(i.hasNext()){
        if(i.next()!=j.next()){
            return false;
        }
    }
    return true;
}//operator==

#//Search
bool QhullPointSet::
contains(const QhullPoint &t) const
{
    QhullPointSetIterator i(*this);
    while(i.hasNext()){
        if(i.next()==t){
            return true;
        }
    }
    return false;
}//contains

int QhullPointSet::
count(const QhullPoint &t) const
{
    int n= 0;
    QhullPointSetIterator i(*this);
    while(i.hasNext()){
        if(i.next()==t){
            ++n;
        }
    }
    return n;
}//count

int QhullPointSet::
indexOf(const QhullPoint &t) const
{
    int idx= 0;
    QhullPointSetIterator i(*this);
    while(i.hasNext()){
        if(i.next()==t){
            return idx;
        }
        ++idx;
    }
    return -1;
}//indexOf

int QhullPointSet::
lastIndexOf(const QhullPoint &t) const
{
    int idx= count()-1;
    QhullPointSetIterator i(*this);
    i.toBack();
    while(i.hasPrevious()){
        if(i.previous()==t){
            break;
        }
        --idx;
    }
    return idx;
}//lastIndexOf


#//QhullPointSetIterator

bool QhullPointSetIterator::
findNext(const QhullPoint &p)
{
    while(i!=c->constEnd()){
        if(*i++ == p){
            return true;
        }
    }
    return false;
}//findNext

bool QhullPointSetIterator::
findPrevious(const QhullPoint &p)
{
    while(i!=c->constBegin()){
        if(*(--i) == p){
            return true;
        }
    }
    return false;
}//findPrevious

}//namespace orgQhull

#//Global functions

using std::endl;
using std::ostream;
using orgQhull::QhullPoint;
using orgQhull::QhullPointSet;
using orgQhull::UsingLibQhull;

#//operator<<

ostream &
operator<<(ostream &os, const QhullPointSet &ps)
{
    os << ps.print(UsingLibQhull::NOqhRunId);
    return os;
}//<<QhullPointSet

ostream &
operator<<(ostream &os, const QhullPointSet::PrintIdentifiers &pr)
{
    const QhullPointSet s= *pr.point_set;
    if (pr.print_message) {
        os << pr.print_message;
    }
    for(QhullPointSet::const_iterator i=s.begin(); i != s.end(); ++i){
        if(i!=s.begin()){
            os << " ";
        }
        const QhullPoint point= *i;
        int id= point.id(pr.run_id);
        os << "p" << id;
    }
    os << endl;
    return os;
}//PrintIdentifiers

ostream &
operator<<(ostream &os, const QhullPointSet::PrintPointSet &pr)
{
    const QhullPointSet s= *pr.point_set;
    if (pr.print_message) {
        os << pr.print_message;
    }
    for(QhullPointSet::const_iterator i=s.begin(); i != s.end(); ++i){
        const QhullPoint point= *i;
        os << point.print(pr.run_id);
    }
    return os;
}//printPointSet


