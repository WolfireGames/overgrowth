/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/RboxPoints.h#6 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef RBOXPOINTS_H
#define RBOXPOINTS_H

#include "QhullPoint.h"
#include "PointCoordinates.h"
extern "C" {
#include "libqhull/libqhull.h"
}

#include <stdarg.h>
#include <string>
#include <vector>
#include <istream>
#include <ostream>
#include <sstream>

namespace orgQhull {

#//Types
    //! RboxPoints -- generate random PointCoordinates for Qhull
    class RboxPoints;

    class RboxPoints : public PointCoordinates {

private:
#//Fields and friends
    int                 rbox_new_count;     //! Number of points for PointCoordinates
    int                 rbox_status;    //! error status from rboxpoints.  qh_ERRnone if none.
    std::string         rbox_message;   //! stderr from rboxpoints

    friend void ::qh_fprintf_rbox(FILE *fp, int msgcode, const char *fmt, ... );

public:
#//Construct
                        RboxPoints();
    explicit            RboxPoints(const char *rboxCommand);
                        RboxPoints(const RboxPoints &other);
                        RboxPoints &operator=(const RboxPoints &other);
                       ~RboxPoints();

public:
#//GetSet
    void                clearRboxMessage();
    int                 newCount() const { return rbox_new_count; }
    std::string         rboxMessage() const;
    int                 rboxStatus() const;
    bool                hasRboxMessage() const;
    void                setNewCount(int pointCount) { QHULL_ASSERT(pointCount>=0); rbox_new_count= pointCount; }

#//Modify
    void                appendPoints(const char* rboxCommand);
    using               PointCoordinates::appendPoints;
    void                reservePoints() { reserveCoordinates((count()+newCount())*dimension()); }
};//class RboxPoints

}//namespace orgQhull

#endif // RBOXPOINTS_H
