/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/PointCoordinates.h#5 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef QHPOINTCOORDINATES_H
#define QHPOINTCOORDINATES_H

#include "QhullPoints.h"
#include "Coordinates.h"
extern "C" {
    #include "libqhull/qhull_a.h"
}

#include <ostream>
#include <vector>

namespace orgQhull {

#//Types
    //! Zero or more points with Coordinates, count, and dimension
    class PointCoordinates;

class PointCoordinates : public QhullPoints {

private:
#//Field
    Coordinates         point_coordinates;      //! array of point coordinates
                                                //! may have extraCoordinates()
    std::string         point_comment;          //! Comment describing PointCoordinates

public:
#//Construct
                        PointCoordinates();
    explicit            PointCoordinates(int pointDimension);
    explicit            PointCoordinates(const std::string &aComment);
                        PointCoordinates(int pointDimension, const std::string &aComment);
                        PointCoordinates(int pointDimension, const std::string &aComment, int coordinatesCount, const coordT *c); // may be invalid
                        //! Use append() and appendPoints() for Coordinates and vector<coordT>
                        PointCoordinates(const PointCoordinates &other);
    PointCoordinates   &operator=(const PointCoordinates &other);
    ~PointCoordinates();

#//Convert
    //! QhullPoints coordinates, constData, data, count, size
#ifndef QHULL_NO_STL
    void                append(const std::vector<coordT> &otherCoordinates) { if(!otherCoordinates.empty()){ append((int)otherCoordinates.size(), &otherCoordinates[0]); } }
    std::vector<coordT> toStdVector() const { return point_coordinates.toStdVector(); }
#endif //QHULL_NO_STL
#ifdef QHULL_USES_QT
    void                append(const QList<coordT> &pointCoordinates) { if(!pointCoordinates.isEmpty()){ append(pointCoordinates.count(), &pointCoordinates[0]); } }
    QList<coordT>       toQList() const { return point_coordinates.toQList(); }
#endif //QHULL_USES_QT

#//GetSet
    //! See QhullPoints for coordinates, coordinateCount, dimension, empty, isEmpty, ==, !=
    void                checkValid() const;
    std::string         comment() const { return point_comment; }
    void                makeValid() { defineAs(point_coordinates.count(), point_coordinates.data()); }
    const Coordinates  &getCoordinates() const { return point_coordinates; }
    void                setComment(const std::string &s) { point_comment= s; }
    void                setDimension(int i);

private:
    void                defineAs(int coordinatesCount, coordT *c) { QhullPoints::defineAs(coordinatesCount, c); }
    //! defineAs() otherwise disabled
public:

#//ElementAccess
    //! See QhullPoints for at, back, first, front, last, mid, [], value

#//Foreach
    //! See QhullPoints for begin, constBegin, end
    Coordinates::ConstIterator  beginCoordinates() const { return point_coordinates.begin(); }
    Coordinates::Iterator       beginCoordinates() { return point_coordinates.begin(); }
    Coordinates::ConstIterator  beginCoordinates(int pointIndex) const;
    Coordinates::Iterator       beginCoordinates(int pointIndex);
    Coordinates::ConstIterator  endCoordinates() const { return point_coordinates.end(); }
    Coordinates::Iterator       endCoordinates() { return point_coordinates.end(); }

#//Search
    //! See QhullPoints for contains, count, indexOf, lastIndexOf

#//Read-only
    PointCoordinates    operator+(const PointCoordinates &other) const;

#//Modify
    //FIXUP QH11001: Add clear() and other modify operators from Coordinates.h.  Include QhullPoint::operator=()
    void                append(int coordinatesCount, const coordT *c);  //! Dimension previously defined
    void                append(const coordT &c) { append(1, &c); } //! Dimension previously defined
    void                append(const QhullPoint &p);
    //! See convert for std::vector and QList
    void                append(const Coordinates &c) { append(c.count(), c.data()); }
    void                append(const PointCoordinates &other);
    void                appendComment(const std::string &s);
    void                appendPoints(std::istream &in);
    PointCoordinates   &operator+=(const PointCoordinates &other) { append(other); return *this; }
    PointCoordinates   &operator+=(const coordT &c) { append(c); return *this; }
    PointCoordinates   &operator+=(const QhullPoint &p) { append(p); return *this; }
    PointCoordinates   &operator<<(const PointCoordinates &other) { return *this += other; }
    PointCoordinates   &operator<<(const coordT &c) { return *this += c; }
    PointCoordinates   &operator<<(const QhullPoint &p) { return *this += p; }
    // reserve() is non-const
    void                reserveCoordinates(int newCoordinates);

#//Helpers
private:
    int                 indexOffset(int i) const;

};//PointCoordinates

// No references to QhullPoint.  Prevents use of QHULL_DECLARE_SEQUENTIAL_ITERATOR(PointCoordinates, QhullPoint)
class PointCoordinatesIterator
{
    typedef PointCoordinates::const_iterator const_iterator;
    const PointCoordinates *c;
    const_iterator i;
    public:
    inline PointCoordinatesIterator(const PointCoordinates &container)
    : c(&container), i(c->constBegin()) {}
    inline PointCoordinatesIterator &operator=(const PointCoordinates &container)
    { c = &container; i = c->constBegin(); return *this; }
    inline void toFront() { i = c->constBegin(); }
    inline void toBack() { i = c->constEnd(); }
    inline bool hasNext() const { return i != c->constEnd(); }
    inline const QhullPoint next() { return *i++; }
    inline const QhullPoint peekNext() const { return *i; }
    inline bool hasPrevious() const { return i != c->constBegin(); }
    inline const QhullPoint previous() { return *--i; }
    inline const QhullPoint peekPrevious() const { const_iterator p = i; return *--p; }
    inline bool findNext(const QhullPoint &t)
    { while (i != c->constEnd()) if (*i++ == t) return true; return false; }
    inline bool findPrevious(const QhullPoint &t)
    { while (i != c->constBegin()) if (*(--i) == t) return true;
    return false;  }
};//CoordinatesIterator

// FIXUP QH11002:  Add MutablePointCoordinatesIterator after adding modify operators
\
}//namespace orgQhull

#//Global functions

std::ostream           &operator<<(std::ostream &os, const orgQhull::PointCoordinates &p);

#endif // QHPOINTCOORDINATES_H
