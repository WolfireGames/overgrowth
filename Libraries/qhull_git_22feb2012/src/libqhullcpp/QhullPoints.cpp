/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullPoints.cpp#4 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#include "QhullPoints.h"

#include <iostream>
#include <algorithm>

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#endif

namespace orgQhull {

#//Conversion
// See qt-qhull.cpp for QList conversion

#ifndef QHULL_NO_STL
std::vector<QhullPoint> QhullPoints::
toStdVector() const
{
    QhullPointsIterator i(*this);
    std::vector<QhullPoint> vs;
    while(i.hasNext()){
        vs.push_back(i.next());
    }
    return vs;
}//toStdVector
#endif //QHULL_NO_STL

#//Read-only

bool QhullPoints::
operator==(const QhullPoints &other) const
{
    if(point_dimension!=other.point_dimension || (point_end-point_first) != (other.point_end-other.point_first)){
        return false;
    }
    const coordT *c= point_first;
    const coordT *c2= other.point_first;
    while(c<point_end){
        if(*c++!=*c2++){
            return false;
        }
    }
    return true;
}//operator==


#//ElementAccess
QhullPoints QhullPoints::
mid(int idx, int length) const
{
    int n= count();
    if(idx<0 || idx>=n){
        n= 0;
    }else if(length<0 || idx+length>=n){
        n -= idx;
    }else{
        n -= idx+length;
    }
    return QhullPoints(point_dimension, n*point_dimension, point_first+idx*point_dimension);
}//mid

QhullPoint QhullPoints::
value(int idx) const
{
    QhullPoint p;
    if(idx>=0 && idx<count()){
        p.defineAs(point_dimension, point_first+idx*point_dimension);
    }
    return p;
}//value

QhullPoint QhullPoints::
value(int idx, QhullPoint &defaultValue) const
{
    QhullPoint p;
    if(idx>=0 && idx<count()){
        p.defineAs(point_dimension, point_first+idx*point_dimension);
    }else{
        p.defineAs(defaultValue);
    }
    return p;
}//value

#//Search

bool QhullPoints::
contains(const QhullPoint &t) const
{
    const_iterator i= begin();
    while(i != end()){
        if(*i==t){
            return true;
        }
        i++;
    }
    return false;
}//contains

int QhullPoints::
count(const QhullPoint &t) const
{
    int n= 0;
    const_iterator i= begin();
    while(i != end()){
        if(*i==t){
            ++n;
        }
        i++;
    }
    return n;
}//count

int QhullPoints::
indexOf(const coordT *pointCoordinates) const
{
    if(!includesCoordinates(pointCoordinates) || dimension()==0){
        return -1;
    }
    size_t offset= pointCoordinates-point_first;
    int idx= (int)(offset/(size_t)dimension()); // int for error reporting
    int extra= (int)(offset%(size_t)dimension());
    if(extra!=0){
        throw QhullError(10066, "Qhull error: coordinates %x are not at point boundary (extra %d at index %d)", extra, idx, 0.0, pointCoordinates);
    }
    return idx;
}//indexOf coordT

int QhullPoints::
indexOf(const coordT *pointCoordinates, int noThrow) const
{
    size_t extra= 0;
    if(noThrow){
        if(!includesCoordinates(pointCoordinates) || dimension()==0){
            return -1;
        }
        extra= (pointCoordinates-point_first)%(size_t)dimension();
    }
    return indexOf(pointCoordinates-extra);
}//indexOf coordT noThrow

int QhullPoints::
indexOf(const QhullPoint &t) const
{
    int j=0;
    const_iterator i= begin();
    while(i!=end()){
        if(*i==t){
            return j;
        }
        ++i;
        ++j;
    }
    return -1;
}//indexOf

int QhullPoints::
lastIndexOf(const QhullPoint &t) const
{
    int j=count();
    const_iterator i= end();
    while(i != begin()){
        --i;
        --j;
        if(*i==t){
            return j;
        }
    }
    return -1;
}//lastIndexOf

#//QhullPointsIterator

bool QhullPointsIterator::
findNext(const QhullPoint &p)
{
    while(i!=ps->constEnd()){
        if(*i++ == p){
            return true;
        }
    }
    return false;
}//findNext

bool QhullPointsIterator::
findPrevious(const QhullPoint &p)
{
    while(i!=ps->constBegin()){
        if(*--i == p){
            return true;
        }
    }
    return false;
}//findPrevious

}//namespace orgQhull

#//Global functions

using std::ostream;
using orgQhull::QhullPoint;
using orgQhull::QhullPoints;
using orgQhull::QhullPointsIterator;

ostream &
operator<<(ostream &os, const QhullPoints &p)
{
    QhullPointsIterator i(p);
    while(i.hasNext()){
        os << i.next();
    }
    return os;
}//operator<<QhullPoints

ostream &
operator<<(ostream &os, const QhullPoints::PrintPoints &pr)
{
    os << pr.point_message;
    QhullPoints ps= *pr.points;
    for(QhullPoints::iterator i=ps.begin(); i != ps.end(); ++i){
        QhullPoint p= *i;
        if(pr.with_identifier){
            os << p.printWithIdentifier(pr.run_id, "");
        }else{
            os << p.print(pr.run_id, "");
        }
    }
    return os;
}//<<PrintPoints
