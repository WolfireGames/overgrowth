/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullSet.h#7 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef QhullSet_H
#define QhullSet_H

#include "QhullError.h"
extern "C" {
    #include "libqhull/qhull_a.h"
}


#ifndef QHULL_NO_STL
#include <vector>
#endif

#ifdef QHULL_USES_QT
 #include <QtCore/QList>
#endif

namespace orgQhull {

#//Type
    class QhullSetBase;  //! Base class for QhullSet<T>
    //! QhullSet<T> -- A read-only wrapper to Qhull's collection class, setT.
    //!  QhullSet is similar to STL's <vector> and Qt's QVector.
    //!  QhullSet is unrelated to STL and Qt's set and map types (e.g., QSet and QMap)
    //!  For STL efficiency, QhullSet caches endPointer()
    //!  T must be a pointer type
    //!  A QhullSet does not own its contents -- erase(), clear(), removeFirst(), removeLast(), pop_back(), pop_front(), fromStdList() not defined
    //!  Qhull's FOREACHelement_() [qset.h] is more efficient than QhullSet.  It uses a NULL terminator instead of an end pointer.  STL requires an end pointer.
    //!  Derived from QhullLinkedList.h and Qt/core/tools/qvector.h

    //! QhullSetIterator<T> defined below
    //See: QhullPointSet, QhullLinkedList<T>

class QhullSetBase {

private:
#//Fields --
    setT               *qh_set;

#//Class objects
    static setT         s_empty_set;  //! Workaround for no setT allocator.  Used if setT* is NULL

public:
#//Class methods
    static int          count(const setT *set);
    //s may be null
    static bool         isEmpty(const setT *s) { return SETempty_(s); }


#//Constructors
                        //! Copy constructor copies the pointer but not the set.  Needed for return by value and parameter passing.
                        QhullSetBase(const QhullSetBase &o) : qh_set(o.qh_set) {}
    explicit            QhullSetBase(setT *s) : qh_set(s ? s : &s_empty_set) {}
                       ~QhullSetBase() {}

private:
                        //!disabled since memory allocation for QhullSet not defined
                        QhullSetBase() {}
                        //!disabled since qs= qs2 is ambiguous (pointer vs. contents)
    QhullSetBase       &operator=(const QhullSetBase &);
public:

#//Conversions
                        //! Not type-safe since setT may contain any type
    void                defineAs(setT *s) { qh_set= s ? s : &s_empty_set; }
    setT               *getSetT() const { return qh_set; }
    setT              **referenceSetT() { return &qh_set; }

#//Read-only
    int                 count() const { return QhullSetBase::count(qh_set); }
    bool                empty() const { return SETfirst_(qh_set)==0; }
    bool                isEmpty() const { return empty(); }
    size_t              size() const { return count(); }

#//Element
protected:
    void              **beginPointer() const { return &qh_set->e[0].p; }
    void              **elementPointer(int idx) const { QHULL_ASSERT(idx>=0 && idx<qh_set->maxsize); return &SETelem_(qh_set, idx); }
                        //! Always points to 0
    void              **endPointer() const { return qh_setendpointer(qh_set); }
};//QhullSetBase


//! set of pointers to baseT, T.getBaseT()
template <typename T>
class QhullSet : public QhullSetBase {

private:
#//Fields -- see QhullSetBase

#//Class objects
    static setT         s_empty_set;  //! Workaround for no setT allocator.  Used if setT* is NULL

public:
#//Subtypes
    typedef T         *iterator;
    typedef const T   *const_iterator;
    typedef typename QhullSet<T>::iterator Iterator;
    typedef typename QhullSet<T>::const_iterator ConstIterator;

#//Class methods
    static int          count(const setT *set);
                        //s may be null
    static bool         isEmpty(const setT *s) { return SETempty_(s); }

#//Constructors
                        //Copy constructor copies pointer but not contents.  Needed for return by value.
                        QhullSet<T>(const QhullSet<T> &o) : QhullSetBase(o) {}
                        //Conversion from setT* is not type-safe.  Implicit conversion for void* to T
    explicit            QhullSet<T>(setT *s) : QhullSetBase(s) { QHULL_ASSERT(sizeof(T)==sizeof(void *)); }
                       ~QhullSet<T>() {}

private:
                        //!Disable default constructor and copy assignment.  See QhullSetBase
                        QhullSet<T>();
    QhullSet<T>        &operator=(const QhullSet<T> &);
public:

#//Conversion

#ifndef QHULL_NO_STL
    std::vector<T>      toStdVector() const;
#endif
#ifdef QHULL_USES_QT
    QList<T>            toQList() const;
#endif

#//Read-only -- see QhullSetBase for count(), empty(), isEmpty(), size()
    using QhullSetBase::count;
    using QhullSetBase::isEmpty;
    // operator== defined for QhullSets of the same type
    bool                operator==(const QhullSet<T> &other) const { return qh_setequal(getSetT(), other.getSetT()); }
    bool                operator!=(const QhullSet<T> &other) const { return !operator==(other); }

#//Element access
    const T            &at(int idx) const { return operator[](idx); }
    T                  &back() { return last(); }
    T                  &back() const { return last(); }
    //! end element is NULL
    const T            *constData() const { return constBegin(); }
    T                  *data() { return begin(); }
    const T            *data() const { return begin(); }
    T                  &first() { QHULL_ASSERT(!isEmpty()); return *begin(); }
    const T            &first() const { QHULL_ASSERT(!isEmpty()); return *begin(); }
    T                  &front() { return first(); }
    const T            &front() const { return first(); }
    T                  &last() { QHULL_ASSERT(!isEmpty()); return *(end()-1); }
    const T            &last() const {  QHULL_ASSERT(!isEmpty()); return *(end()-1); }
    // mid() not available.  No setT constructor
    T                  &operator[](int idx) { T *n= reinterpret_cast<T *>(elementPointer(idx)); QHULL_ASSERT(idx>=0 && n < reinterpret_cast<T *>(endPointer())); return *n; }
    const T            &operator[](int idx) const { const T *n= reinterpret_cast<const T *>(elementPointer(idx)); QHULL_ASSERT(idx>=0 && n < reinterpret_cast<T *>(endPointer())); return *n; }
    T                  &second() { return operator[](1); }
    const T            &second() const { return operator[](1); }
    T                   value(int idx) const;
    T                   value(int idx, const T &defaultValue) const;

#//Read-write -- Not available, no setT constructor

#//iterator
    iterator            begin() { return iterator(beginPointer()); }
    const_iterator      begin() const { return const_iterator(beginPointer()); }
    const_iterator      constBegin() const { return const_iterator(beginPointer()); }
    const_iterator      constEnd() const { return const_iterator(endPointer()); }
    iterator            end() { return iterator(endPointer()); }
    const_iterator      end() const { return const_iterator(endPointer()); }

#//Search
    bool                contains(const T &t) const;
    int                 count(const T &t) const;
    int                 indexOf(const T &t) const { /* no qh_qh */ return qh_setindex(getSetT(), t.getBaseT()); }
    int                 lastIndexOf(const T &t) const;

};//class QhullSet

// FIXUP? can't use QHULL_DECLARE_SEQUENTIAL_ITERATOR because it is not a template

template <typename T>
class QhullSetIterator {

#//Subtypes
    typedef typename QhullSet<T>::const_iterator const_iterator;

private:
#//Fields
    const_iterator      i;
    const_iterator      begin_i;
    const_iterator      end_i;

public:
#//Constructors
                        QhullSetIterator<T>(const QhullSet<T> &s) : i(s.begin()), begin_i(i), end_i(s.end()) {}
                        QhullSetIterator<T>(const QhullSetIterator<T> &o) : i(o.i), begin_i(o.begin_i), end_i(o.end_i) {}
    QhullSetIterator<T> &operator=(const QhullSetIterator<T> &o) { i= o.i; begin_i= o.begin_i; end_i= o.end_i; return *this; }

#//ReadOnly
    int                 countRemaining() { return (int)(end_i-begin_i); } // WARN64

#//Search
    bool                findNext(const T &t);
    bool                findPrevious(const T &t);

#//Foreach
    bool                hasNext() const { return i != end_i; }
    bool                hasPrevious() const { return i != begin_i; }
    T                   next() { return *i++; }
    T                   peekNext() const { return *i; }
    T                   peekPrevious() const { const_iterator p = i; return *--p; }
    T                   previous() { return *--i; }
    void                toBack() { i = end_i; }
    void                toFront() { i = begin_i; }
};//class QhullSetIterator

#//== Definitions =========================================

#//Conversion

#ifndef QHULL_NO_STL
template <typename T>
std::vector<T> QhullSet<T>::
toStdVector() const
{
    QhullSetIterator<T> i(*this);
    std::vector<T> vs;
    vs.reserve(i.countRemaining());
    while(i.hasNext()){
        vs.push_back(i.next());
    }
    return vs;
}//toStdVector
#endif

#ifdef QHULL_USES_QT
template <typename T>
QList<T> QhullSet<T>::
toQList() const
{
    QhullSetIterator<T> i(*this);
    QList<T> vs;
    while(i.hasNext()){
        vs.append(i.next());
    }
    return vs;
}//toQList
#endif

#//Element

template <typename T>
T QhullSet<T>::
value(int idx) const
{
    // Avoid call to qh_setsize() and assert in elementPointer()
    const T *n= reinterpret_cast<const T *>(&SETelem_(getSetT(), idx));
    return (idx>=0 && n<end()) ? *n : T();
}//value

template <typename T>
T QhullSet<T>::
value(int idx, const T &defaultValue) const
{
    // Avoid call to qh_setsize() and assert in elementPointer()
    const T *n= reinterpret_cast<const T *>(&SETelem_(getSetT(), idx));
    return (idx>=0 && n<end()) ? *n : defaultValue;
}//value

#//Search

template <typename T>
bool QhullSet<T>::
contains(const T &t) const
{
    setT *s= getSetT();
    void *e= t.getBaseT();  // contains() is not inline for better error reporting
    int result= qh_setin(s, e);
    return result!=0;
}//contains

template <typename T>
int QhullSet<T>::
count(const T &t) const
{
    int c= 0;
    const T *i= data();
    const T *e= end();
    while(i<e){
        if(*i==t){
            c++;
        }
        i++;
    }
    return c;
}//count

template <typename T>
int QhullSet<T>::
lastIndexOf(const T &t) const
{
    const T *b= begin();
    const T *i= end();
    while(--i>=b){
        if(*i==t){
            break;
        }
    }
    return (int)(i-b); // WARN64
}//lastIndexOf

#//QhullSetIterator

template <typename T>
bool QhullSetIterator<T>::
findNext(const T &t)
{
    while(i!=end_i){
        if(*(++i)==t){
            return true;
        }
    }
    return false;
}//findNext

template <typename T>
bool QhullSetIterator<T>::
findPrevious(const T &t)
{
    while(i!=begin_i){
        if(*(--i)==t){
            return true;
        }
    }
    return false;
}//findPrevious

}//namespace orgQhull


#//== Global namespace =========================================

template <typename T>
std::ostream &
operator<<(std::ostream &os, const orgQhull::QhullSet<T> &qs)
{
    const T *i= qs.begin();
    const T *e= qs.end();
    while(i!=e){
        os << *i;
        ++i;
    }
    return os;
}//operator<<

#endif // QhullSet_H
