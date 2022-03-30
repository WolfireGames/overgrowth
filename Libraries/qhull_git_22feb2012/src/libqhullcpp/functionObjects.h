/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/functionObjects.h#4 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef QHFUNCTIONOBJECTS_H
#define QHFUNCTIONOBJECTS_H

#include <stdlib.h>
#include <math.h>

namespace orgQhull {
#//Type

    //! Sum of absolute values of the elements in a container
    class AbsoluteSumOf;
    //! Sum of the elements in a container
    class SumOf;
    //! Sum of squares of the elements in a container
    class SumSquaresOf;

#//Class

//! Absolute sum of the elements in a container
class AbsoluteSumOf
{
private:
    double sum;
public:
    inline AbsoluteSumOf() : sum(0.0) {}
    inline void operator()(double v) { sum += fabs(v); }
    inline operator double() { return sum; }
};//AbsoluteSumOf

//! Sum of the elements in a container
class SumOf
{
private:
    double sum;
public:
    inline SumOf() : sum(0.0) {}
    inline void operator()(double v) { sum += v; }
    inline operator double() { return sum; }
};//SumOf


//! Sum of squares of the elements in a container
class SumSquaresOf
{
private:
    double sum;
public:
    inline SumSquaresOf() : sum(0.0) {}
    inline void operator()(double v) { sum += v*v; }
    inline operator double() { return sum; }
};//SumSquaresOf


}//orgQhull


#endif //QHFUNCTIONOBJECTS_H

