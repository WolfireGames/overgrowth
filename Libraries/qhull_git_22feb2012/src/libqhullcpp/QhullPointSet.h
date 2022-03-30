/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullPointSet.h#6 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef QHULLPOINTSET_H
#define QHULLPOINTSET_H

#include "QhullSet.h"
#include "QhullPoint.h"
extern "C" {
    #include "libqhull/qhull_a.h"
}

#include <ostream>

namespace orgQhull {

#//Types
    //! QhullPointSet -- a set of coordinate pointers with dimension
    // with const_iterator and iterator
    class               QhullPointSet;
    //! Java-style iterator
    class QhullPointsIterator;

#//Classref
    class               QhullPoint;

class QhullPointSet : public QhullSet<coordT *> {

private:
#//Field
    int                 point_dimension;

public:
#//Subtypes and types
    class               const_iterator;
    class               iterator;
    typedef QhullPointSet::const_iterator ConstIterator;
    typedef QhullPointSet::iterator Iterator;

    typedef QhullPoint  value_type;
    typedef ptrdiff_t   difference_type;
    typedef int         size_type;
    //typedef const value_type *const_pointer;    // FIXUP QH11019: QhullPointSet does not define pointer or reference due to point_dimension
    //typedef const value_type &const_reference;
    //typedef value_type *pointer;
    //typedef value_type &reference;

#//Construct
                        //Conversion from setT* is not type-safe.  Implicit conversion for void* to T
                        QhullPointSet(int pointDimension, setT *s) : QhullSet<coordT *>(s), point_dimension(pointDimension) {}
                        //Copy constructor copies pointer but not contents.  Needed for return by value and parameter passing.
                        QhullPointSet(const QhullPointSet &o) : QhullSet<coordT *>(o), point_dimension(o.point_dimension) {}
                       ~QhullPointSet() {}

//Default constructor and copy assignment disabled since p= p2 is ambiguous (coord* vs coord)
private:
                        QhullPointSet();
    QhullPointSet      &operator=(const QhullPointSet &);
public:

#//Conversions
    // inherited -- constData, data
#ifndef QHULL_NO_STL
    std::vector<QhullPoint> toStdVector() const;
#endif
#ifdef QHULL_USES_QT
    QList<QhullPoint>   toQList() const;
#endif

#//Read-only
    //inherits count, empty, isEmpty, size
    using QhullSetBase::count;
    int                 dimension() const { return point_dimension; }
    bool                operator==(const QhullPointSet &o) const;
    bool                operator!=(const QhullPointSet &o) const { return !operator==(o); }

#//Element access -- can not return references since QhullPoint must be generated
    QhullPoint          at(int idx) const { return operator[](idx); }
    QhullPoint          back() const { return last(); }
    //! end element is NULL
    QhullPoint          first() const { QHULL_ASSERT(!isEmpty()); return *begin(); }
    QhullPoint          front() const { return first(); }
    QhullPoint          last() const { QHULL_ASSERT(!isEmpty()); return *(end()-1); }
    // mid() not available.  No setT constructor
    QhullPoint          operator[](int idx) const { return QhullPoint(dimension(), QhullSet<coordT *>::operator[](idx)); }
    QhullPoint          second()  const { return operator[](1); }
    QhullPoint          value(int idx) const;
    // Non-const since copy is an alias
    QhullPoint          value(int idx, QhullPoint &defaultValue) const;

#//iterator
    iterator            begin() { return iterator(dimension(), reinterpret_cast<coordT **>(beginPointer())); }
    const_iterator      begin() const { return const_iterator(dimension(), reinterpret_cast<coordT **>(beginPointer())); }
    const_iterator      constBegin() const { return const_iterator(dimension(), reinterpret_cast<coordT **>(beginPointer())); }
    const_iterator      constEnd() const { return const_iterator(dimension(), reinterpret_cast<coordT **>(endPointer())); }
    iterator            end() { return iterator(dimension(), reinterpret_cast<coordT **>(endPointer())); }
    const_iterator      end() const { return const_iterator(dimension(), reinterpret_cast<coordT **>(endPointer())); }

//Read-write -- Not available, no setT constructor

#//Search
    bool                contains(const QhullPoint &t) const;
    int                 count(const QhullPoint &t) const;
    int                 indexOf(const QhullPoint &t) const;
    int                 lastIndexOf(const QhullPoint &t) const;

    // before const_iterator for conversion with comparison operators
    class iterator {
        friend class    const_iterator;

    private:
        coordT        **i;
        int             point_dimension;

    public:
        typedef ptrdiff_t   difference_type;
        typedef std::bidirectional_iterator_tag  iterator_category;
        typedef QhullPoint *pointer;
        typedef QhullPoint &reference;
        typedef QhullPoint  value_type;

                        iterator() : i(0), point_dimension(0) {}
                        iterator(int dimension, coordT **c) : i(c), point_dimension(dimension) {}
                        iterator(const iterator &o) : i(o.i), point_dimension(o.point_dimension) {}
        iterator       &operator=(const iterator &o) { i= o.i; point_dimension= o.point_dimension; return *this; }

        QhullPoint      operator*() const { return QhullPoint(point_dimension, *i); }
                      //operator->() n/a, value-type
        QhullPoint      operator[](int idx) { return QhullPoint(point_dimension, *(i+idx)); }
        bool            operator==(const iterator &o) const { return i == o.i && point_dimension == o.point_dimension; }
        bool            operator!=(const iterator &o) const { return !operator==(o); }
        bool            operator==(const const_iterator &o) const
        { return i == reinterpret_cast<const iterator &>(o).i && point_dimension == reinterpret_cast<const iterator &>(o).point_dimension; }
        bool            operator!=(const const_iterator &o) const { return !operator==(o); }

        //! Assumes same point set
        int             operator-(const iterator &o) { return (int)(i-o.i); } //WARN64
        bool            operator>(const iterator &o) const { return i>o.i; }
        bool            operator<=(const iterator &o) const { return !operator>(o); }
        bool            operator<(const iterator &o) const { return i<o.i; }
        bool            operator>=(const iterator &o) const { return !operator<(o); }
        bool            operator>(const const_iterator &o) const
        { return i > reinterpret_cast<const iterator &>(o).i; }
        bool            operator<=(const const_iterator &o) const { return !operator>(o); }
        bool            operator<(const const_iterator &o) const
        { return i < reinterpret_cast<const iterator &>(o).i; }
        bool            operator>=(const const_iterator &o) const { return !operator<(o); }

        iterator       &operator++() { ++i; return *this; }
        iterator        operator++(int) { iterator o= *this; ++i; return o; }
        iterator       &operator--() { --i; return *this; }
        iterator        operator--(int) { iterator o= *this; --i; return o; }
        iterator        operator+(int j) const { return iterator(point_dimension, i+j); }
        iterator        operator-(int j) const { return operator+(-j); }
        iterator       &operator+=(int j) { i += j; return *this; }
        iterator       &operator-=(int j) { i -= j; return *this; }
    };//QhullPointSet::iterator

    class const_iterator {
    private:
        coordT        **i;
        int             point_dimension;

    public:
        typedef std::random_access_iterator_tag  iterator_category;
        typedef QhullPoint value_type;
        typedef value_type *pointer;
        typedef value_type &reference;
        typedef ptrdiff_t  difference_type;

                        const_iterator() : i(0), point_dimension(0) {}
                        const_iterator(int dimension, coordT **c) : i(c), point_dimension(dimension) {}
                        const_iterator(const const_iterator &o) : i(o.i), point_dimension(o.point_dimension) {}
                        const_iterator(iterator o) : i(o.i), point_dimension(o.point_dimension) {}
        const_iterator &operator=(const const_iterator &o) { i= o.i; point_dimension= o.point_dimension; return *this; }

        QhullPoint      operator*() const { return QhullPoint(point_dimension, *i); }
        QhullPoint      operator[](int idx) { return QhullPoint(point_dimension, *(i+idx)); }
                      //operator->() n/a, value-type
        bool            operator==(const const_iterator &o) const { return i == o.i && point_dimension == o.point_dimension; }
        bool            operator!=(const const_iterator &o) const { return !operator==(o); }

        //! Assumes same point set
        int             operator-(const const_iterator &o) { return (int)(i-o.i); } //WARN64
        bool            operator>(const const_iterator &o) const { return i>o.i; }
        bool            operator<=(const const_iterator &o) const { return !operator>(o); }
        bool            operator<(const const_iterator &o) const { return i<o.i; }
        bool            operator>=(const const_iterator &o) const { return !operator<(o); }

        const_iterator &operator++() { ++i; return *this; }
        const_iterator  operator++(int) { const_iterator o= *this; ++i; return o; }
        const_iterator &operator--() { --i; return *this; }
        const_iterator  operator--(int) { const_iterator o= *this; --i; return o; }
        const_iterator  operator+(int j) const { return const_iterator(point_dimension, i+j); }
        const_iterator  operator-(int j) const { return operator+(-j); }
        const_iterator &operator+=(int j) { i += j; return *this; }
        const_iterator &operator-=(int j) { i -= j; return *this; }
    };//QhullPointSet::const_iterator

#//IO
    struct PrintIdentifiers{
        const QhullPointSet *point_set;
        const char     *print_message;
        int             run_id;
        PrintIdentifiers(const char *message, const QhullPointSet *s) : point_set(s), print_message(message) {}
    };//PrintIdentifiers
    PrintIdentifiers printIdentifiers(const char *message) const { return PrintIdentifiers(message, this); }

    struct PrintPointSet{
        const QhullPointSet *point_set;
        const char     *print_message;
        int             run_id;
        PrintPointSet(int qhRunId, const char *message, const QhullPointSet &s) : point_set(&s), print_message(message), run_id(qhRunId) {}
    };//PrintPointSet
    PrintPointSet       print(int qhRunId) const { return PrintPointSet(qhRunId, 0, *this); }
    PrintPointSet       print(int qhRunId, const char *message) const { return PrintPointSet(qhRunId, message, *this); }

};//QhullPointSet

//derived from qiterator.h
class QhullPointSetIterator { // FIXUP QH11020 define QhullMutablePointSetIterator
    typedef QhullPointSet::const_iterator const_iterator;
    const QhullPointSet *c;
    const_iterator      i;

public:
                        QhullPointSetIterator(const QhullPointSet &container) : c(&container), i(c->constBegin()) {}
    QhullPointSetIterator &operator=(const QhullPointSet &container) { c= &container; i= c->constBegin(); return *this; }
    bool                findNext(const QhullPoint &p);
    bool                findPrevious(const QhullPoint &p);
    bool                hasNext() const { return i != c->constEnd(); }
    bool                hasPrevious() const { return i != c->constBegin(); }
    QhullPoint          next() { return *i++; }
    QhullPoint          peekNext() const { return *i; }
    QhullPoint          peekPrevious() const { const_iterator p= i; return *--p; }
    QhullPoint          previous() { return *--i; }
    void                toBack() { i= c->constEnd(); }
    void                toFront() { i= c->constBegin(); }
};//QhullPointSetIterator

}//namespace orgQhull

#//Global functions

std::ostream &operator<<(std::ostream &os, const orgQhull::QhullPointSet &fs); // Not inline to avoid using statement
std::ostream &operator<<(std::ostream &os, const orgQhull::QhullPointSet::PrintIdentifiers &pr);
std::ostream &operator<<(std::ostream &os, const orgQhull::QhullPointSet::PrintPointSet &pr);

#endif // QHULLPOINTSET_H
