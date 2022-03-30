/****************************************************************************
**
** Copyright (c) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullHyperplane.h#6 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef QHHYPERPLANE_H
#define QHHYPERPLANE_H

#include "QhullError.h"
#include "QhullIterator.h"
#include "UsingLibQhull.h"
extern "C" {
    #include "libqhull/qhull_a.h"
}

#include <ostream>

namespace orgQhull {
#//ClassRef
    class QhullPoint;

#//Types
    //! QhullHyperplane as an offset, dimension, and pointer to coordinates
    class QhullHyperplane;
    //! Java-style iterator for QhullHyperplane coordinates
    class QhullHyperplaneIterator;

class QhullHyperplane { // Similar to QhullPoint

private:
#//Fields
    coordT             *hyperplane_coordinates;  // Keep pointers aligned
    int                 hyperplane_dimension;
    coordT              hyperplane_offset;

public:
#//Subtypes
    typedef const coordT *                  iterator;
    typedef const coordT *                  const_iterator;
    typedef QhullHyperplane::iterator       Iterator;
    typedef QhullHyperplane::const_iterator ConstIterator;

#//Construct
                        QhullHyperplane() : hyperplane_coordinates(0), hyperplane_dimension(0), hyperplane_offset(0.0) {};
                        QhullHyperplane(int hyperplaneDimension, coordT *c, coordT hyperplaneOffset) : hyperplane_coordinates(c), hyperplane_dimension(hyperplaneDimension), hyperplane_offset(hyperplaneOffset) {}
                        // Creates an alias.  Does not copy the hyperplane's coordinates.  Needed for return by value and parameter passing.
                        QhullHyperplane(const QhullHyperplane &other)  : hyperplane_coordinates(other.hyperplane_coordinates), hyperplane_dimension(other.hyperplane_dimension), hyperplane_offset(other.hyperplane_offset) {}
                        // Creates an alias.  Does not copy the hyperplane's coordinates.  Needed for vector<QhullHyperplane>
    QhullHyperplane    &operator=(const QhullHyperplane &other) { hyperplane_coordinates= other.hyperplane_coordinates; hyperplane_dimension= other.hyperplane_dimension; hyperplane_offset= other.hyperplane_offset; return *this; }
                       ~QhullHyperplane() {}

#//Conversions --
//! Includes offset at end
#ifndef QHULL_NO_STL
    std::vector<coordT> toStdVector() const;
#endif //QHULL_NO_STL
#ifdef QHULL_USES_QT
    QList<coordT>       toQList() const;
#endif //QHULL_USES_QT

#//Read-only
public:
    const coordT       *coordinates() const { return hyperplane_coordinates; }
    coordT             *coordinates() { return hyperplane_coordinates; }
    int                 dimension() const { return hyperplane_dimension; }
    bool                isDefined() const { return hyperplane_coordinates!=0 && hyperplane_dimension>0; }
    coordT              offset() const { return hyperplane_offset; }

#//Define
    void                defineAs(int hyperplaneDimension, coordT *c, coordT hyperplaneOffset) { QHULL_ASSERT(hyperplaneDimension>=0); hyperplane_coordinates= c; hyperplane_dimension= hyperplaneDimension; hyperplane_offset= hyperplaneOffset; }
    //! Creates an alias to other
    void                defineAs(QhullHyperplane &other) { hyperplane_coordinates= other.coordinates(); hyperplane_dimension= other.dimension();  hyperplane_offset= other.offset(); }
    void                setCoordinates(coordT *c) { hyperplane_coordinates= c; }
    void                setDimension(int hyperplaneDimension) { hyperplane_dimension= hyperplaneDimension; }
    void                setOffset(coordT hyperplaneOffset) { hyperplane_offset= hyperplaneOffset; }

#//value
    double              distance(const QhullPoint &p) const;
    double              norm() const;

#//iterator
    iterator            begin() { return hyperplane_coordinates; }
    const_iterator      begin() const { return hyperplane_coordinates; }
    const_iterator      constBegin() const { return hyperplane_coordinates; }
    const_iterator      constEnd() const { return hyperplane_coordinates+hyperplane_dimension; }
    int                 count() { return dimension(); }
    iterator            end() { return hyperplane_coordinates+hyperplane_dimension; }
    const_iterator      end() const { return hyperplane_coordinates+hyperplane_dimension; }
    size_t              size() { return (size_t)dimension(); }

#//Operator
    bool                operator==(const QhullHyperplane &other) const;
    bool                operator!=(const QhullHyperplane &other) const { return !operator==(other); }
    const coordT       &operator[](int idx) const { QHULL_ASSERT(idx>=0 && idx<hyperplane_dimension); return *(hyperplane_coordinates+idx); }
    coordT             &operator[](int idx) { QHULL_ASSERT(idx>=0 && idx<hyperplane_dimension); return *(hyperplane_coordinates+idx); }

#//IO
    struct PrintHyperplane{
        const QhullHyperplane  *hyperplane;  
        const char     *print_message;
        const char     *hyperplane_offset_message;
                        PrintHyperplane(const char *message, const char *offsetMessage, const QhullHyperplane &p) : hyperplane(&p), print_message(message), hyperplane_offset_message(offsetMessage) {}
    };//PrintHyperplane
    PrintHyperplane          print() const { return  PrintHyperplane(0, 0, *this); }
    PrintHyperplane          print(const char *message, const char *offsetMessage) const { return PrintHyperplane(message, offsetMessage, *this); }

};//QhullHyperplane

QHULL_DECLARE_SEQUENTIAL_ITERATOR(QhullHyperplane, coordT)

}//namespace orgQhull

#//Global functions

std::ostream &operator<<(std::ostream &os, const orgQhull::QhullHyperplane::PrintHyperplane &pr);
std::ostream &operator<<(std::ostream &os, const orgQhull::QhullHyperplane &p); //FIXUP QH11015 -- multiple instances if define here

#endif // QHHYPERPLANE_H

