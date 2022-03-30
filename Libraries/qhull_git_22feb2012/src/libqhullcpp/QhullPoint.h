/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullPoint.h#6 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef QHPOINT_H
#define QHPOINT_H

#include "QhullError.h"
#include "QhullIterator.h"
#include "UsingLibQhull.h"
#include "Coordinates.h"
extern "C" {
    #include "libqhull/qhull_a.h"
}

#include <ostream>

namespace orgQhull {

#//Types
    //! QhullPoint as a pointer and dimension to shared memory
    class QhullPoint;
    //! Java-style iterator for QhullPoint coordinates
    class QhullPointIterator;

class QhullPoint {

    //! A point is a pointer into an array of coordinates.

private:
#//Fields
    coordT             *point_coordinates;  // Keep pointers aligned
    int                 point_dimension;

public:
#//Subtypes
    typedef const coordT *              iterator;
    typedef const coordT *              const_iterator;
    typedef QhullPoint::iterator        Iterator;
    typedef QhullPoint::const_iterator  ConstIterator;

#//Class methods -- Convert point to id w/o QhullQh data structure
    static int          id(const coordT *c) { return QhullPoint::id(UsingLibQhull::NOqhRunId, 0, c); }
    static int          id(int qhRunId, const coordT *c) { return QhullPoint::id(qhRunId, 0, c); }
    static int          id(int qhRunId, int dimension, const coordT *c);

#//Construct
                        QhullPoint() : point_coordinates(0), point_dimension(0) {};
                        QhullPoint(int pointDimension, coordT *c) : point_coordinates(c), point_dimension(pointDimension) {}
    explicit            QhullPoint(Coordinates &c) : point_coordinates(c.data()), point_dimension(c.count()) {}
                        // Creates an alias.  Does not copy the point.  Needed for return by value and parameter passing.
                        QhullPoint(const QhullPoint &other)  : point_coordinates(other.point_coordinates), point_dimension(other.point_dimension) {}
                        // Creates an alias.  Does not copy the point.  Needed for vector<QhullPoint>
    QhullPoint         &operator=(const QhullPoint &other) { point_coordinates= other.point_coordinates; point_dimension= other.point_dimension; return *this; }
                       ~QhullPoint() {}

#//Conversions
    // see coordinates()
#ifndef QHULL_NO_STL
    std::vector<coordT> toStdVector() const;
#endif //QHULL_NO_STL
#ifdef QHULL_USES_QT
    QList<coordT>       toQList() const;
#endif //QHULL_USES_QT

#//Read-only
public:
    const coordT       *coordinates() const { return point_coordinates; }
    coordT             *coordinates() { return point_coordinates; }
    int                 dimension() const { return point_dimension; }
    int                 id(int qhRunId) const { return id(qhRunId, dimension(), coordinates()); }
    int                 id() const { return id(UsingLibQhull::NOqhRunId, dimension(), coordinates()); }
    bool                isDefined() const { return point_coordinates!=0 && point_dimension>0; }

#//Define
    void                advancePoint(int idx) { point_coordinates += idx*point_dimension; }
    void                defineAs(int pointDimension, coordT *c) { QHULL_ASSERT(pointDimension>=0); point_coordinates= c; point_dimension= pointDimension; }
    //! Creates an alias to other
    void                defineAs(QhullPoint &other) { point_coordinates= other.coordinates(); point_dimension= other.dimension(); }
    void                setCoordinates(coordT *c) { point_coordinates= c; }
    void                setDimension(int pointDimension) { point_dimension= pointDimension; }

#//value
    double              distance(const QhullPoint &p) const;

#//iterator
    iterator            begin() { return point_coordinates; }
    const_iterator      begin() const { return point_coordinates; }
    const_iterator      constBegin() const { return point_coordinates; }
    const_iterator      constEnd() const { return point_coordinates+point_dimension; }
    int                 count() { return dimension(); }
    iterator            end() { return point_coordinates+point_dimension; }
    const_iterator      end() const { return point_coordinates+point_dimension; }
    size_t              size() { return (size_t)dimension(); }

#//Operator
    bool                operator==(const QhullPoint &other) const;
    bool                operator!=(const QhullPoint &other) const { return !operator==(other); }
    const coordT       &operator[](int idx) const { QHULL_ASSERT(idx>=0 && idx<point_dimension); return *(point_coordinates+idx); }
    coordT             &operator[](int idx) { QHULL_ASSERT(idx>=0 && idx<point_dimension); return *(point_coordinates+idx); }

    struct PrintPoint{
        const QhullPoint  *point;
        const char     *point_message;
        int             run_id;
        bool            with_identifier;
                        PrintPoint(int qhRunId, const char *message, bool withIdentifier, const QhullPoint &p) : point(&p), point_message(message), run_id(qhRunId), with_identifier(withIdentifier) {}
    };//PrintPoint
    PrintPoint          print() const { return  PrintPoint(UsingLibQhull::NOqhRunId, "", false, *this); }
    PrintPoint          print(int qhRunId) const { return PrintPoint(qhRunId, "", true, *this); }
    PrintPoint          print(int qhRunId, const char *message) const { return PrintPoint(qhRunId, message, false, *this); }
    PrintPoint          printWithIdentifier(int qhRunId, const char *message) const { return PrintPoint(qhRunId, message, true, *this); }

};//QhullPoint

QHULL_DECLARE_SEQUENTIAL_ITERATOR(QhullPoint, coordT)

}//namespace orgQhull

#//Global functions

std::ostream &operator<<(std::ostream &os, const orgQhull::QhullPoint::PrintPoint &pr);
std::ostream &operator<<(std::ostream &os, const orgQhull::QhullPoint &p); // FIXUP QH11017 OK in c program but not inline { os << p.print(orgQhull::UsingLibQhull::NOqhRunId, ""); return os; }

#endif // QHPOINT_H

