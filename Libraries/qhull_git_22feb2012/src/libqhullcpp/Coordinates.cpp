/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/Coordinates.cpp#3 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#include "functionObjects.h"
#include "QhullError.h"
#include "Coordinates.h"

#include <iostream>
#include <iterator>
#include <algorithm>

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#endif

namespace orgQhull {

#//! Coordinates -- vector of coordT (normally double)

#//Element access

// Inefficient without result-value-optimization or implicitly shared object
Coordinates Coordinates::
mid(int idx, int length) const
{
    int newLength= length;
    if(length<0 || idx+length > count()){
        newLength= count()-idx;
    }
    Coordinates result;
    if(newLength>0){
        std::copy(begin()+idx, begin()+(idx+newLength), std::back_inserter(result));
    }
    return result;
}//mid

coordT Coordinates::
value(int idx, const coordT &defaultValue) const
{
    return ((idx < 0 || idx >= count()) ? defaultValue : (*this)[idx]);
}//value

#//Operator

Coordinates Coordinates::
operator+(const Coordinates &other) const
{
    Coordinates result(*this);
    std::copy(other.begin(), other.end(), std::back_inserter(result));
    return result;
}//operator+

Coordinates & Coordinates::
operator+=(const Coordinates &other)
{
    if(&other==this){
        Coordinates clone(other);
        std::copy(clone.begin(), clone.end(), std::back_inserter(*this));
    }else{
        std::copy(other.begin(), other.end(), std::back_inserter(*this));
    }
    return *this;
}//operator+=

#//Read-write

coordT Coordinates::
takeAt(int idx)
{
    coordT c= at(idx);
    erase(begin()+idx);
    return c;
}//takeAt

coordT Coordinates::
takeLast()
{
    coordT c= last();
    removeLast();
    return c;
}//takeLast

void Coordinates::
swap(int idx, int other)
{
    coordT c= at(idx);
    at(idx)= at(other);
    at(other)= c;
}//swap

#//Search

bool Coordinates::
contains(const coordT &t) const
{
    CoordinatesIterator i(*this);
    return i.findNext(t);
}//contains

int Coordinates::
count(const coordT &t) const
{
    CoordinatesIterator i(*this);
    int result= 0;
    while(i.findNext(t)){
        ++result;
    }
    return result;
}//count

int Coordinates::
indexOf(const coordT &t, int from) const
{
    if(from<0){
        from += count();
        if(from<0){
            from= 0;
        }
    }
    if(from<count()){
        const_iterator i= begin()+from;
        while(i!=constEnd()){
            if(*i==t){
                return (static_cast<int>(i-begin())); // WARN64
            }
            ++i;
        }
    }
    return -1;
}//indexOf

int Coordinates::
lastIndexOf(const coordT &t, int from) const
{
    if(from<0){
        from += count();
    }else if(from>=count()){
        from= count()-1;
    }
    if(from>=0){
        const_iterator i= begin()+from+1;
        while(i-- != constBegin()){
            if(*i==t){
                return (static_cast<int>(i-begin())); // WARN64
            }
        }
    }
    return -1;
}//lastIndexOf

void Coordinates::
removeAll(const coordT &t)
{
    MutableCoordinatesIterator i(*this);
    while(i.findNext(t)){
        i.remove();
    }
}//removeAll

}//namespace orgQhull

#//Global functions

using std::endl;
using std::istream;
using std::ostream;
using std::string;
using std::ws;
using orgQhull::Coordinates;

ostream &
operator<<(ostream &os, const Coordinates &cs)
{
    Coordinates::const_iterator c= cs.begin();
    for(int i=cs.count(); i--; ){
        os << *c++ << " ";
    }
    return os;
}//operator<<

