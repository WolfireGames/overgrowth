/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullPoint.cpp#4 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#include "UsingLibQhull.h"
#include "QhullPoint.h"

#include <iostream>
#include <algorithm>

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#endif

namespace orgQhull {

#//Class public variables and methods

//! If qhRundID undefined uses QhullPoint::s_points_begin and dimension
int QhullPoint::
id(int qhRunId, int dimension, const coordT *c)
{
    QHULL_UNUSED(dimension);

    if(UsingLibQhull::hasPoints()){
        if(qhRunId==UsingLibQhull::NOqhRunId){
            const coordT *pointsEnd;
            int dim;
            const coordT *points= UsingLibQhull::globalPoints(&dim, &pointsEnd);
            if(c>=points && c<pointsEnd){
                int offset= (int)(c-points); // WARN64
                return offset/dim;
            }
        }else{
            UsingLibQhull q(qhRunId);
            // NOerrors from qh_pointid or qh_setindex
            return qh_pointid(const_cast<coordT *>(c));
        }
    }
    long long i=(long long)c;
    return (int)i; // WARN64
}//id

#//Conversion

// See qt-qhull.cpp for QList conversion

#ifndef QHULL_NO_STL
std::vector<coordT> QhullPoint::
toStdVector() const
{
    QhullPointIterator i(*this);
    std::vector<coordT> vs;
    while(i.hasNext()){
        vs.push_back(i.next());
    }
    return vs;
}//toStdVector
#endif //QHULL_NO_STL

#//Operator

bool QhullPoint::
operator==(const QhullPoint &other) const
{
    if(point_dimension!=other.point_dimension){
        return false;
    }
    const coordT *c= point_coordinates;
    const coordT *c2= other.point_coordinates;
    if(c==c2){
        return true;
    }
    double dist2= 0.0;
    for(int k= point_dimension; k--; ){
        double diff= *c++ - *c2++;
        dist2 += diff*diff;
    }
    double epsilon= UsingLibQhull::globalDistanceEpsilon();
    // std::cout << "DEBUG dist2 " << dist2 << " epsilon^2 " << epsilon*epsilon << std::endl;
    return (dist2<=(epsilon*epsilon));
}//operator==


#//Value

//! Return distance betweeen two points.
double QhullPoint::
distance(const QhullPoint &p) const
{
    const coordT *c= coordinates();
    const coordT *c2= p.coordinates();
    int dim= dimension();
    QHULL_ASSERT(dim==p.dimension());
    double dist;

    switch(dim){
  case 2:
      dist= (c[0]-c2[0])*(c[0]-c2[0]) + (c[1]-c2[1])*(c[1]-c2[1]);
      break;
  case 3:
      dist= (c[0]-c2[0])*(c[0]-c2[0]) + (c[1]-c2[1])*(c[1]-c2[1]) + (c[2]-c2[2])*(c[2]-c2[2]);
      break;
  case 4:
      dist= (c[0]-c2[0])*(c[0]-c2[0]) + (c[1]-c2[1])*(c[1]-c2[1]) + (c[2]-c2[2])*(c[2]-c2[2]) + (c[3]-c2[3])*(c[3]-c2[3]);
      break;
  case 5:
      dist= (c[0]-c2[0])*(c[0]-c2[0]) + (c[1]-c2[1])*(c[1]-c2[1]) + (c[2]-c2[2])*(c[2]-c2[2]) + (c[3]-c2[3])*(c[3]-c2[3]) + (c[4]-c2[4])*(c[4]-c2[4]);
      break;
  case 6:
      dist= (c[0]-c2[0])*(c[0]-c2[0]) + (c[1]-c2[1])*(c[1]-c2[1]) + (c[2]-c2[2])*(c[2]-c2[2]) + (c[3]-c2[3])*(c[3]-c2[3]) + (c[4]-c2[4])*(c[4]-c2[4]) + (c[5]-c2[5])*(c[5]-c2[5]);
      break;
  case 7:
      dist= (c[0]-c2[0])*(c[0]-c2[0]) + (c[1]-c2[1])*(c[1]-c2[1]) + (c[2]-c2[2])*(c[2]-c2[2]) + (c[3]-c2[3])*(c[3]-c2[3]) + (c[4]-c2[4])*(c[4]-c2[4]) + (c[5]-c2[5])*(c[5]-c2[5]) + (c[6]-c2[6])*(c[6]-c2[6]);
      break;
  case 8:
      dist= (c[0]-c2[0])*(c[0]-c2[0]) + (c[1]-c2[1])*(c[1]-c2[1]) + (c[2]-c2[2])*(c[2]-c2[2]) + (c[3]-c2[3])*(c[3]-c2[3]) + (c[4]-c2[4])*(c[4]-c2[4]) + (c[5]-c2[5])*(c[5]-c2[5]) + (c[6]-c2[6])*(c[6]-c2[6]) + (c[7]-c2[7])*(c[7]-c2[7]);
      break;
  default:
      dist= 0.0;
      for(int k=dim; k--; ){
          dist += (*c - *c2) * (*c - *c2);
          ++c;
          ++c2;
      }
      break;
    }
    return sqrt(dist);
}//distance

}//namespace orgQhull

#//Global functions

using std::ostream;
using orgQhull::QhullPoint;
using orgQhull::UsingLibQhull;

#//operator<<

ostream &
operator<<(ostream &os, const QhullPoint &p)
{
    os << p.printWithIdentifier(UsingLibQhull::NOqhRunId, "");
    return os;
}

//! Same as qh_printpointid [io.c]
ostream &
operator<<(ostream &os, const QhullPoint::PrintPoint &pr)
{
    QhullPoint p= *pr.point; 
    int i= p.id(pr.run_id);
    if(pr.point_message){
        if(*pr.point_message){
            os << pr.point_message << " ";
        }
        if(pr.with_identifier && (i!=-1)){
            os << "p" << i << ": ";
        }
    }
    const realT *c= p.coordinates();
    for(int k=p.dimension(); k--; ){
        realT r= *c++;
        if(pr.point_message){
            os << " " << r; // FIXUP QH11010 %8.4g
        }else{
            os << " " << r; // FIXUP QH11010 qh_REAL_1
        }
    }
    os << std::endl;
    return os;
}//printPoint

