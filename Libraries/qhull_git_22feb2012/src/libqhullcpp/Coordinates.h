/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/Coordinates.h#6 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef QHCOORDINATES_H
#define QHCOORDINATES_H

#include "QhullError.h"
#include "QhullIterator.h"
extern "C" {
    #include "libqhull/qhull_a.h"
}


#include <cstddef> // ptrdiff_t, size_t
#include <ostream>
#include <vector>

namespace orgQhull {

#//Types
    //! an allocated vector of point coordinates
    //!  Used by PointCoordinates for RboxPoints
    //!  A QhullPoint refers to previously allocated coordinates
    class  Coordinates;
    class  MutableCoordinatesIterator;


class Coordinates {

private:
#//Fields
    std::vector<coordT> coordinate_array;

public:
#//Subtypes

    class                       const_iterator;
    class                       iterator;
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;

    typedef coordT              value_type;
    typedef const value_type   *const_pointer;
    typedef const value_type   &const_reference;
    typedef value_type         *pointer;
    typedef value_type         &reference;
    typedef ptrdiff_t           difference_type;
    typedef int                 size_type;

#//Construct
                        Coordinates() {};
    explicit            Coordinates(const std::vector<coordT> &other) : coordinate_array(other) {}
                        Coordinates(const Coordinates &other) : coordinate_array(other.coordinate_array) {}
    Coordinates        &operator=(const Coordinates &other) { coordinate_array= other.coordinate_array; return *this; }
    Coordinates        &operator=(const std::vector<coordT> &other) { coordinate_array= other; return *this; }
                       ~Coordinates() {}

#//Conversion

    coordT             *data() { return isEmpty() ? 0 : &at(0); }
    const coordT       *data() const { return const_cast<const pointT*>(isEmpty() ? 0 : &at(0)); }

#ifndef QHULL_NO_STL
    std::vector<coordT> toStdVector() const { return coordinate_array; }
#endif //QHULL_NO_STL
#ifdef QHULL_USES_QT
    QList<coordT>      toQList() const;
#endif //QHULL_USES_QT

#//GetSet
    int                count() const { return static_cast<int>(size()); }
    bool               empty() const { return coordinate_array.empty(); }
    bool               isEmpty() const { return empty(); }
    bool               operator==(const Coordinates &other) const  { return coordinate_array==other.coordinate_array; }
    bool               operator!=(const Coordinates &other) const  { return coordinate_array!=other.coordinate_array; }
    size_t             size() const { return coordinate_array.size(); }

#//Element access
    coordT             &at(int idx) { return coordinate_array.at(idx); }
    const coordT       &at(int idx) const { return coordinate_array.at(idx); }
    coordT             &back() { return coordinate_array.back(); }
    const coordT       &back() const { return coordinate_array.back(); }
    coordT             &first() { return front(); }
    const coordT       &first() const { return front(); }
    coordT             &front() { return coordinate_array.front(); }
    const coordT       &front() const { return coordinate_array.front(); }
    coordT             &last() { return back(); }
    const coordT       &last() const { return back(); }
    Coordinates        mid(int idx, int length= -1) const;
    coordT            &operator[](int idx) { return coordinate_array.operator[](idx); }
    const coordT      &operator[](int idx) const { return coordinate_array.operator[](idx); }
    coordT             value(int idx, const coordT &defaultValue) const;

#//Iterator
    iterator            begin() { return iterator(coordinate_array.begin()); }
    const_iterator      begin() const { return const_iterator(coordinate_array.begin()); }
    const_iterator      constBegin() const { return begin(); }
    const_iterator      constEnd() const { return end(); }
    iterator            end() { return iterator(coordinate_array.end()); }
    const_iterator      end() const { return const_iterator(coordinate_array.end()); }

#//Read-only
    Coordinates         operator+(const Coordinates &other) const;

#//Modify
    void                append(const coordT &c) { push_back(c); }
    void                clear() { coordinate_array.clear(); }
    iterator            erase(iterator idx) { return iterator(coordinate_array.erase(idx.base())); }
    iterator            erase(iterator beginIterator, iterator endIterator) { return iterator(coordinate_array.erase(beginIterator.base(), endIterator.base())); }
    void                insert(int before, const coordT &c) { insert(begin()+before, c); }
    iterator            insert(iterator before, const coordT &c) { return iterator(coordinate_array.insert(before.base(), c)); }
    void                move(int from, int to) { insert(to, takeAt(from)); }
    Coordinates        &operator+=(const Coordinates &other);
    Coordinates        &operator+=(const coordT &c) { append(c); return *this; }
    Coordinates        &operator<<(const Coordinates &other) { return *this += other; }
    Coordinates        &operator<<(const coordT &c) { return *this += c; }
    void                pop_back() { coordinate_array.pop_back(); }
    void                pop_front() { removeFirst(); }
    void                prepend(const coordT &c) { insert(begin(), c); }
    void                push_back(const coordT &c) { coordinate_array.push_back(c); }
    void                push_front(const coordT &c) { insert(begin(), c); }
                        //removeAll below
    void                removeAt(int idx) { erase(begin()+idx); }
    void                removeFirst() { erase(begin()); }
    void                removeLast() { erase(--end()); }
    void                replace(int idx, const coordT &c) { (*this)[idx]= c; }
    void                reserve(int i) { coordinate_array.reserve(i); }
    void                swap(int idx, int other);
    coordT              takeAt(int idx);
    coordT              takeFirst() { return takeAt(0); }
    coordT              takeLast();

#//Search
    bool                contains(const coordT &t) const;
    int                 count(const coordT &t) const;
    int                 indexOf(const coordT &t, int from = 0) const;
    int                 lastIndexOf(const coordT &t, int from = -1) const;
    void                removeAll(const coordT &t);

#//Coordinates::iterator -- from QhullPoints, forwarding to coordinate_array
    // before const_iterator for conversion with comparison operators
    class iterator {

    private:
        std::vector<coordT>::iterator i;
        friend class    const_iterator;

    public:
        typedef std::random_access_iterator_tag  iterator_category;
        typedef coordT      value_type;
        typedef value_type *pointer;
        typedef value_type &reference;
        typedef ptrdiff_t   difference_type;

                        iterator() {}
                        iterator(const iterator &other) { i= other.i; }
        explicit        iterator(const std::vector<coordT>::iterator &vi) { i= vi; }
        iterator       &operator=(const iterator &other) { i= other.i; return *this; }
        std::vector<coordT>::iterator &base() { return i; }
                        // No operator-> for base types
        coordT         &operator*() const { return *i; }
        coordT         &operator[](int idx) const { return i[idx]; }

        bool            operator==(const iterator &other) const { return i==other.i; }
        bool            operator!=(const iterator &other) const { return i!=other.i; }
        bool            operator<(const iterator &other) const { return i<other.i; }
        bool            operator<=(const iterator &other) const { return i<=other.i; }
        bool            operator>(const iterator &other) const { return i>other.i; }
        bool            operator>=(const iterator &other) const { return i>=other.i; }
              // reinterpret_cast to break circular dependency
        bool            operator==(const Coordinates::const_iterator &other) const { return *this==reinterpret_cast<const iterator &>(other); }
        bool            operator!=(const Coordinates::const_iterator &other) const { return *this!=reinterpret_cast<const iterator &>(other); }
        bool            operator<(const Coordinates::const_iterator &other) const { return *this<reinterpret_cast<const iterator &>(other); }
        bool            operator<=(const Coordinates::const_iterator &other) const { return *this<=reinterpret_cast<const iterator &>(other); }
        bool            operator>(const Coordinates::const_iterator &other) const { return *this>reinterpret_cast<const iterator &>(other); }
        bool            operator>=(const Coordinates::const_iterator &other) const { return *this>=reinterpret_cast<const iterator &>(other); }

        iterator        operator++() { return iterator(++i); } //FIXUP QH11012 Should return reference, but get reference to temporary
        iterator        operator++(int) { return iterator(i++); }
        iterator        operator--() { return iterator(--i); }
        iterator        operator--(int) { return iterator(i--); }
        iterator        operator+=(int idx) { return iterator(i += idx); }
        iterator        operator-=(int idx) { return iterator(i -= idx); }
        iterator        operator+(int idx) const { return iterator(i+idx); }
        iterator        operator-(int idx) const { return iterator(i-idx); }
        difference_type operator-(iterator other) const { return i-other.i; }
    };//Coordinates::iterator

#//Coordinates::const_iterator
    class const_iterator {

    private:
        std::vector<coordT>::const_iterator i;

    public:
        typedef std::random_access_iterator_tag  iterator_category;
        typedef coordT            value_type;
        typedef const value_type *pointer;
        typedef const value_type &reference;
        typedef ptrdiff_t         difference_type;

                        const_iterator() {}
                        const_iterator(const const_iterator &other) { i= other.i; }
                        const_iterator(iterator o) : i(o.i) {}
        explicit        const_iterator(const std::vector<coordT>::const_iterator &vi) { i= vi; }
        const_iterator &operator=(const const_iterator &other) { i= other.i; return *this; }
                        // No operator-> for base types
                        // No reference to a base type for () and []
        const coordT   &operator*() const { return *i; }
        const coordT   &operator[](int idx) const { return i[idx]; }

        bool            operator==(const const_iterator &other) const { return i==other.i; }
        bool            operator!=(const const_iterator &other) const { return i!=other.i; }
        bool            operator<(const const_iterator &other) const { return i<other.i; }
        bool            operator<=(const const_iterator &other) const { return i<=other.i; }
        bool            operator>(const const_iterator &other) const { return i>other.i; }
        bool            operator>=(const const_iterator &other) const { return i>=other.i; }

        const_iterator  operator++() { return const_iterator(++i); } //FIXUP QH11014 -- too much copying
        const_iterator  operator++(int) { return const_iterator(i++); }
        const_iterator  operator--() { return const_iterator(--i); }
        const_iterator  operator--(int) { return const_iterator(i--); }
        const_iterator  operator+=(int idx) { return const_iterator(i += idx); }
        const_iterator  operator-=(int idx) { return const_iterator(i -= idx); }
        const_iterator  operator+(int idx) const { return const_iterator(i+idx); }
        const_iterator  operator-(int idx) const { return const_iterator(i-idx); }
        difference_type operator-(const_iterator other) const { return i-other.i; }
    };//Coordinates::const_iterator

};//Coordinates

//class CoordinatesIterator
//QHULL_DECLARE_SEQUENTIAL_ITERATOR(Coordinates, coordT)

class CoordinatesIterator
{
    typedef Coordinates::const_iterator const_iterator;
    const Coordinates *c;
    const_iterator i;
    public:
    inline CoordinatesIterator(const Coordinates &container)
    : c(&container), i(c->constBegin()) {}
    inline CoordinatesIterator &operator=(const Coordinates &container)
    { c = &container; i = c->constBegin(); return *this; }
    inline void toFront() { i = c->constBegin(); }
    inline void toBack() { i = c->constEnd(); }
    inline bool hasNext() const { return i != c->constEnd(); }
    inline const coordT &next() { return *i++; }
    inline const coordT &peekNext() const { return *i; }
    inline bool hasPrevious() const { return i != c->constBegin(); }
    inline const coordT &previous() { return *--i; }
    inline const coordT &peekPrevious() const { const_iterator p = i; return *--p; }
    inline bool findNext(const coordT &t)
    { while (i != c->constEnd()) if (*i++ == t) return true; return false; }
    inline bool findPrevious(const coordT &t)
    { while (i != c->constBegin()) if (*(--i) == t) return true;
    return false;  }
};//CoordinatesIterator

//class MutableCoordinatesIterator
//QHULL_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR(Coordinates, coordT)
class MutableCoordinatesIterator
{
    typedef Coordinates::iterator iterator;
    typedef Coordinates::const_iterator const_iterator;
    Coordinates *c;
    iterator i, n;
    inline bool item_exists() const { return const_iterator(n) != c->constEnd(); }
    public:
    inline MutableCoordinatesIterator(Coordinates &container)
    : c(&container)
    { i = c->begin(); n = c->end(); }
    inline ~MutableCoordinatesIterator()
    {}
    inline MutableCoordinatesIterator &operator=(Coordinates &container)
    { c = &container;
    i = c->begin(); n = c->end(); return *this; }
    inline void toFront() { i = c->begin(); n = c->end(); }
    inline void toBack() { i = c->end(); n = i; }
    inline bool hasNext() const { return c->constEnd() != const_iterator(i); }
    inline coordT &next() { n = i++; return *n; }
    inline coordT &peekNext() const { return *i; }
    inline bool hasPrevious() const { return c->constBegin() != const_iterator(i); }
    inline coordT &previous() { n = --i; return *n; }
    inline coordT &peekPrevious() const { iterator p = i; return *--p; }
    inline void remove()
    { if (c->constEnd() != const_iterator(n)) { i = c->erase(n); n = c->end(); } }
    inline void setValue(const coordT &t) const { if (c->constEnd() != const_iterator(n)) *n = t; }
    inline coordT &value() { QHULL_ASSERT(item_exists()); return *n; }
    inline const coordT &value() const { QHULL_ASSERT(item_exists()); return *n; }
    inline void insert(const coordT &t) { n = i = c->insert(i, t); ++i; }
    inline bool findNext(const coordT &t)
    { while (c->constEnd() != const_iterator(n = i)) if (*i++ == t) return true; return false; }
    inline bool findPrevious(const coordT &t)
    { while (c->constBegin() != const_iterator(i)) if (*(n = --i) == t) return true;
    n = c->end(); return false;  }
};//MutableCoordinatesIterator


}//namespace orgQhull

#//Global functions

std::ostream &operator<<(std::ostream &os, const orgQhull::Coordinates &c);

#endif // QHCOORDINATES_H
