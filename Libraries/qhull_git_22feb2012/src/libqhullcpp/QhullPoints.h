/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullPoints.h#5 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef QHULLPOINTS_H
#define QHULLPOINTS_H

#include "QhullPoint.h"
extern "C" {
    #include "libqhull/qhull_a.h"
}

#include <ostream>

namespace orgQhull {

#//Types
    //! coordinate pointer with dimension
    // with const_iterator and iterator
    class QhullPoints;
    //! Java-style iterator
    class QhullPointsIterator;

class QhullPoints {

    // QhullPoints consists of pointers into an array of coordinates.

private:
#//Field
    coordT             *point_first;
    coordT             *point_end;  // end>=first.  Trailing coordinates ignored
    int                 point_dimension;  // >= 0

public:
#//Subtypes
    class               const_iterator;
    class               iterator;
    typedef QhullPoints::const_iterator ConstIterator;
    typedef QhullPoints::iterator Iterator;

#//Construct
                        QhullPoints() : point_first(0), point_end(0), point_dimension(0) {};
                        QhullPoints(int pointDimension) : point_first(0), point_end(0), point_dimension(pointDimension) { QHULL_ASSERT(pointDimension>=0); }
                        QhullPoints(int pointDimension, int coordinateCount2, coordT *c) : point_first(c), point_end(c+coordinateCount2), point_dimension(pointDimension) { QHULL_ASSERT(pointDimension>=0 && coordinateCount2>=0 ); }
                        //Copy constructor copies pointers but not contents.  Needed for return by value and parameter passing.
                        QhullPoints(const QhullPoints &other)  : point_first(other.point_first), point_end(other.point_end), point_dimension(other.point_dimension) {}
                       ~QhullPoints() {}

//disabled since p= p2 is ambiguous (coord* vs coord)
private:
    QhullPoints        &operator=(const QhullPoints &other) { point_first= other.point_first; point_end= other.point_end; point_dimension= other.point_dimension; return *this; }
public:

#//Conversion
    const coordT       *constData() const { return coordinates(); }
    // See coordinates()
    coordT             *data() { return coordinates(); }
    const coordT       *data() const { return coordinates(); }
#ifndef QHULL_NO_STL
    std::vector<QhullPoint> toStdVector() const;
#endif //QHULL_NO_STL
#ifdef QHULL_USES_QT
    QList<QhullPoint>   toQList() const;
#endif //QHULL_USES_QT

#//GetSet
    coordT             *coordinates() const { return point_first; }
    int                 coordinateCount() const { return (int)(point_end-point_first); } // WARN64
    int                 count() const { return (int)size(); } // WARN64
    void                defineAs(int pointDimension, int coordinatesCount, coordT *c) { QHULL_ASSERT(pointDimension>=0 && coordinatesCount>=0 && c!=0); point_first= c; point_end= c+coordinatesCount; point_dimension= pointDimension; }
    void                defineAs(int coordinatesCount, coordT *c) { QHULL_ASSERT((coordinatesCount>=0 && c!=0) || (c==0 && coordinatesCount==0)); point_first= c; point_end= c+coordinatesCount; }
    void                defineAs(const QhullPoints &other) { point_first= other.point_first; point_end= other.point_end; point_dimension= other.point_dimension; }
    int                 dimension() const { return point_dimension; }
    bool                empty() const { return point_end==point_first; }
    coordT             *extraCoordinates() const { return extraCoordinatesCount() ? (point_end-extraCoordinatesCount()) : 0; }
    int                 extraCoordinatesCount() const { return point_dimension>0 ? (int)((point_end-point_first)%(size_t)point_dimension) : 0; }  // WARN64
    bool                includesCoordinates(const coordT *c) const { return c>=point_first && c<point_end; }
    bool                isEmpty() const { return empty(); }
    bool                operator==(const QhullPoints &other) const;
    bool                operator!=(const QhullPoints &other) const { return !operator==(other); }
    void                setDimension(int pointDimension) { QHULL_ASSERT(pointDimension>=0); point_dimension= pointDimension; }
    size_t              size() const { return (point_dimension ? (point_end-point_first)/point_dimension : 0); }

#//ElementAccess -- can not return references to QhullPoint
    QhullPoint          at(int idx) const { coordT *p= point_first+idx*point_dimension; QHULL_ASSERT(p<point_end); return QhullPoint(point_dimension, p); }
    QhullPoint          back() const { return last(); }
    QhullPoint          first() const { return QhullPoint(point_dimension, point_first); }
    QhullPoint          front() const { return first(); }
    QhullPoint          last() const { return QhullPoint(point_dimension, point_end - point_dimension); }
    //! Returns a subset of the points, not a copy
    QhullPoints         mid(int idx, int length= -1) const;
    QhullPoint          operator[](int idx) const { return at(idx); }
    QhullPoint          value(int idx) const;
    // Non-const since copy is an alias
    QhullPoint          value(int idx, QhullPoint &defaultValue) const;

#//Foreach
    ConstIterator       begin() const { return ConstIterator(*this); }
    Iterator            begin() { return Iterator(*this); }
    ConstIterator       constBegin() const { return ConstIterator(*this); }
    ConstIterator       constEnd() const { return ConstIterator(point_dimension, point_end); }
    ConstIterator       end() const { return ConstIterator(point_dimension, point_end); }
    Iterator            end() { return Iterator(point_dimension, point_end); }

#//Search
    bool                contains(const QhullPoint &t) const;
    int                 count(const QhullPoint &t) const;
    int                 indexOf(const coordT *pointCoordinates) const;
    int                 indexOf(const coordT *pointCoordinates, int noThrow) const;
    int                 indexOf(const QhullPoint &t) const;
    int                 lastIndexOf(const QhullPoint &t) const;

#//QhullPoints::iterator -- modeled on qvector.h and qlist.h
    // before const_iterator for conversion with comparison operators
    // See: QhullSet.h
    class iterator : public QhullPoint {

    public:
        typedef std::random_access_iterator_tag  iterator_category;
        typedef QhullPoint  value_type;
        typedef value_type *pointer;
        typedef value_type &reference;
        typedef ptrdiff_t   difference_type;

                        iterator() : QhullPoint() {}
                        iterator(const iterator &other): QhullPoint(*other) {}
        explicit        iterator(const QhullPoints &ps) : QhullPoint(ps.dimension(), ps.coordinates()) {}
        explicit        iterator(int pointDimension, coordT *c): QhullPoint(pointDimension, c) {}
        iterator       &operator=(const iterator &other) { defineAs( const_cast<iterator &>(other)); return *this; }
        QhullPoint     *operator->() { return this; }
        // value instead of reference since advancePoint() modifies self
        QhullPoint      operator*() const { return *this; }
        QhullPoint      operator[](int idx) const { QhullPoint n= *this; n.advancePoint(idx); return n; }
        bool            operator==(const iterator &other) const { QHULL_ASSERT(dimension()==other.dimension()); return coordinates()==other.coordinates(); }
        bool            operator!=(const iterator &other) const { return !operator==(other); }
        bool            operator<(const iterator &other) const  { QHULL_ASSERT(dimension()==other.dimension()); return coordinates() < other.coordinates(); }
        bool            operator<=(const iterator &other) const { QHULL_ASSERT(dimension()==other.dimension()); return coordinates() <= other.coordinates(); }
        bool            operator>(const iterator &other) const  { QHULL_ASSERT(dimension()==other.dimension()); return coordinates() > other.coordinates(); }
        bool            operator>=(const iterator &other) const { QHULL_ASSERT(dimension()==other.dimension()); return coordinates() >= other.coordinates(); }
        // reinterpret_cast to break circular dependency
        bool            operator==(const QhullPoints::const_iterator &other) const { QHULL_ASSERT(dimension()==reinterpret_cast<const iterator &>(other).dimension()); return coordinates()==reinterpret_cast<const iterator &>(other).coordinates(); }
        bool            operator!=(const QhullPoints::const_iterator &other) const { return !operator==(reinterpret_cast<const iterator &>(other)); }
        bool            operator<(const QhullPoints::const_iterator &other) const  { QHULL_ASSERT(dimension()==reinterpret_cast<const iterator &>(other).dimension()); return coordinates() < reinterpret_cast<const iterator &>(other).coordinates(); }
        bool            operator<=(const QhullPoints::const_iterator &other) const { QHULL_ASSERT(dimension()==reinterpret_cast<const iterator &>(other).dimension()); return coordinates() <= reinterpret_cast<const iterator &>(other).coordinates(); }
        bool            operator>(const QhullPoints::const_iterator &other) const  { QHULL_ASSERT(dimension()==reinterpret_cast<const iterator &>(other).dimension()); return coordinates() > reinterpret_cast<const iterator &>(other).coordinates(); }
        bool            operator>=(const QhullPoints::const_iterator &other) const { QHULL_ASSERT(dimension()==reinterpret_cast<const iterator &>(other).dimension()); return coordinates() >= reinterpret_cast<const iterator &>(other).coordinates(); }
        iterator       &operator++() { advancePoint(1); return *this; }
        iterator        operator++(int) { iterator n= *this; operator++(); return iterator(n); }
        iterator       &operator--() { advancePoint(-1); return *this; }
        iterator        operator--(int) { iterator n= *this; operator--(); return iterator(n); }
        iterator       &operator+=(int idx) { advancePoint(idx); return *this; }
        iterator       &operator-=(int idx) { advancePoint(-idx); return *this; }
        iterator        operator+(int idx) const { iterator n= *this; n.advancePoint(idx); return n; }
        iterator        operator-(int idx) const { iterator n= *this; n.advancePoint(-idx); return n; }
        difference_type operator-(iterator other) const { QHULL_ASSERT(dimension()==other.dimension()); return (coordinates()-other.coordinates())/dimension(); }
    };//QhullPoints::iterator

#//QhullPoints::const_iterator -- FIXUP QH11018 const_iterator same as iterator
    class const_iterator : public QhullPoint {

    public:
        typedef std::random_access_iterator_tag  iterator_category;
        typedef QhullPoint          value_type;
        typedef const value_type   *pointer;
        typedef const value_type   &reference;
        typedef ptrdiff_t           difference_type;

                        const_iterator() : QhullPoint() {}
                        const_iterator(const const_iterator &other) : QhullPoint(*other) {}
                        const_iterator(const QhullPoints::iterator &other) : QhullPoint(*other) {}
        explicit        const_iterator(const QhullPoints &ps) : QhullPoint(ps.dimension(), ps.coordinates()) {}
        explicit        const_iterator(int pointDimension, coordT *c): QhullPoint(pointDimension, c) {}
        const_iterator &operator=(const const_iterator &other) { defineAs(const_cast<const_iterator &>(other)); return *this; }
        // value/non-const since advancePoint(1), etc. modifies self
        QhullPoint      operator*() const { return *this; }
        QhullPoint     *operator->() { return this; }
        QhullPoint      operator[](int idx) const { QhullPoint n= *this; n.advancePoint(idx); return n; }
        bool            operator==(const const_iterator &other) const { QHULL_ASSERT(dimension()==other.dimension()); return coordinates()==other.coordinates(); }
        bool            operator!=(const const_iterator &other) const { return !operator==(other); }
        bool            operator<(const const_iterator &other) const  { QHULL_ASSERT(dimension()==other.dimension()); return coordinates() < other.coordinates(); }
        bool            operator<=(const const_iterator &other) const { QHULL_ASSERT(dimension()==other.dimension()); return coordinates() <= other.coordinates(); }
        bool            operator>(const const_iterator &other) const  { QHULL_ASSERT(dimension()==other.dimension()); return coordinates() > other.coordinates(); }
        bool            operator>=(const const_iterator &other) const { QHULL_ASSERT(dimension()==other.dimension()); return coordinates() >= other.coordinates(); }
        const_iterator &operator++() { advancePoint(1); return *this; }
        const_iterator  operator++(int) { const_iterator n= *this; operator++(); return const_iterator(n); }
        const_iterator &operator--() { advancePoint(-1); return *this; }
        const_iterator  operator--(int) { const_iterator n= *this; operator--(); return const_iterator(n); }
        const_iterator &operator+=(int idx) { advancePoint(idx); return *this; }
        const_iterator &operator-=(int idx) { advancePoint(-idx); return *this; }
        const_iterator  operator+(int idx) const { const_iterator n= *this; n.advancePoint(idx); return n; }
        const_iterator  operator-(int idx) const { const_iterator n= *this; n.advancePoint(-idx); return n; }
        difference_type operator-(const_iterator other) const { QHULL_ASSERT(dimension()==other.dimension()); return (coordinates()-other.coordinates())/dimension(); }
    };//QhullPoints::const_iterator

#//IO
    struct PrintPoints{
        const QhullPoints  *points;
        const char     *point_message;
        int             run_id;
        bool            with_identifier;
        PrintPoints(int qhRunId, const char *message, bool withIdentifier, const QhullPoints &ps) : points(&ps), point_message(message), run_id(qhRunId), with_identifier(withIdentifier) {}
    };//PrintPoints
    PrintPoints          print() const { return  PrintPoints(UsingLibQhull::NOqhRunId, "", false, *this); }
    PrintPoints          print(int qhRunId) const { return PrintPoints(qhRunId, "", true, *this); }
    PrintPoints          print(int qhRunId, const char *message) const { return PrintPoints(qhRunId, message, false, *this); }
    PrintPoints          printWithIdentifier(int qhRunId, const char *message) const { return PrintPoints(qhRunId, message, true, *this); }
    //FIXUP remove message for print()?
};//QhullPoints

// can't use QHULL_DECLARE_SEQUENTIAL_ITERATOR because next(),etc would return a reference to a temporary
class QhullPointsIterator
{
    typedef QhullPoints::const_iterator const_iterator;

private:
#//Fields
    const QhullPoints  *ps;
    const_iterator      i;

public:
                        QhullPointsIterator(const QhullPoints &other) : ps(&other), i(ps->constBegin()) {}
    QhullPointsIterator &operator=(const QhullPoints &other) { ps = &other; i = ps->constBegin(); return *this; }
    bool                findNext(const QhullPoint &t);
    bool                findPrevious(const QhullPoint &t);
    bool                hasNext() const { return i != ps->constEnd(); }
    bool                hasPrevious() const { return i != ps->constBegin(); }
    QhullPoint          next() { return *i++; }
    QhullPoint          peekNext() const { return *i; }
    QhullPoint          peekPrevious() const { const_iterator p = i; return *--p; }
    QhullPoint          previous() { return *--i; }
    void                toBack() { i = ps->constEnd(); }
    void                toFront() { i = ps->constBegin(); }
};//QhullPointsIterator

}//namespace orgQhull

#//Global functions

std::ostream           &operator<<(std::ostream &os, const orgQhull::QhullPoints &p);
std::ostream           &operator<<(std::ostream &os, const orgQhull::QhullPoints::PrintPoints &pr);

#endif // QHULLPOINTS_H
