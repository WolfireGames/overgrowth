/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/QhullSets.h#3 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef QHULLSETS_H
#define QHULLSETS_H

#include "QhullSet.h"

namespace orgQhull {

    //See: QhullFacetSet.h
    //See: QhullPointSet.h
    //See: QhullVertexSet.h

    // Avoid circular references between QhullFacet, QhullRidge, and QhullVertex
    class QhullRidge;
    typedef QhullSet<QhullRidge>  QhullRidgeSet;
    typedef QhullSetIterator<QhullRidge>  QhullRidgeSetIterator;

}//namespace orgQhull

#endif // QHULLSETS_H
