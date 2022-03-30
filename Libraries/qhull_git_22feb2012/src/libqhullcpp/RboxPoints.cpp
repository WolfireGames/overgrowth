/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/RboxPoints.cpp#4 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#include "QhullError.h"
#include "RboxPoints.h"

#include <iostream>

using std::cerr;
using std::endl;
using std::istream;
using std::ostream;
using std::ostringstream;
using std::string;
using std::vector;
using std::ws;

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#pragma warning( disable : 4996)  // function was declared deprecated(strcpy, localtime, etc.)
#endif

namespace orgQhull {

#//! RboxPoints -- generate random PointCoordinates for qhull (rbox)

#//Global

//! pointer to RboxPoints for qh_fprintf callback
RboxPoints *rbox_output= 0;

#//Construct
RboxPoints::
RboxPoints()
: PointCoordinates("rbox")
, rbox_new_count(0)
, rbox_status(qh_ERRnone)
, rbox_message()
{}

RboxPoints::
RboxPoints(const char *rboxCommand)
: PointCoordinates("rbox")
, rbox_new_count(0)
, rbox_status(qh_ERRnone)
, rbox_message()
{
    appendPoints(rboxCommand);
}

RboxPoints::
RboxPoints(const RboxPoints &other)
: PointCoordinates(other)
, rbox_new_count(0)
, rbox_status(other.rbox_status)
, rbox_message(other.rbox_message)
{}

RboxPoints & RboxPoints::
operator=(const RboxPoints &other)
{
    PointCoordinates::operator=(other);
    rbox_new_count= other.rbox_new_count;
    rbox_status= other.rbox_status;
    rbox_message= other.rbox_message;
    return *this;
}//operator=


RboxPoints::
~RboxPoints()
{}

#//Error

void RboxPoints::
clearRboxMessage()
{
    rbox_status= qh_ERRnone;
    rbox_message.clear();
}//clearRboxMessage

std::string RboxPoints::
rboxMessage() const
{
    if(rbox_status!=qh_ERRnone){
        return rbox_message;
    }
    if(isEmpty()){
        return "rbox warning: no points generated\n";
    }
    return "rbox: OK\n";
}//rboxMessage

int RboxPoints::
rboxStatus() const
{
    return rbox_status;
}

bool RboxPoints::
hasRboxMessage() const
{
    return (rbox_status!=qh_ERRnone);
}

#//Modify

void RboxPoints::
appendPoints(const char *rboxCommand)
{
    string s("rbox ");
    s += rboxCommand;
    char *command= const_cast<char*>(s.c_str());
    if(rbox_output){
        throw QhullError(10001, "Qhull error: Two simultaneous calls to RboxPoints::appendPoints().  Prevent two processes calling appendPoints() at the same time.  Other RboxPoints '%s'", 0, 0, 0, rbox_output->comment().c_str());
    }
    if(extraCoordinatesCount()!=0){
        throw QhullError(10067, "Qhull error: Extra coordinates (%d) prior to calling RboxPoints::appendPoints.  Was %s", extraCoordinatesCount(), 0, 0.0, comment().c_str());
    }
    int previousCount= count();
    rbox_output= this;              // set rbox_output for qh_fprintf()
    int status= ::qh_rboxpoints(0, 0, command);
    rbox_output= 0;
    if(rbox_status==qh_ERRnone){
        rbox_status= status;
    }
    if(rbox_status!=qh_ERRnone){
        throw QhullError(rbox_status, rbox_message);
    }
    if(extraCoordinatesCount()!=0){
        throw QhullError(10002, "Qhull error: extra coordinates (%d) for PointCoordinates (%x)", extraCoordinatesCount(), 0, 0.0, coordinates());
    }
    if(previousCount+newCount()!=count()){
        throw QhullError(10068, "Qhull error: rbox specified %d points but got %d points for command '%s'", newCount(), count()-previousCount, 0.0, comment().c_str());
    }
}//appendPoints

}//namespace orgQhull

#//Global functions

/*-<a                             href="qh-user.htm#TOC"
>-------------------------------</a><a name="qh_fprintf_rbox">-</a>

  qh_fprintf_rbox(fp, msgcode, format, list of args )
    fp is ignored (replaces qh_fprintf_rbox() in userprintf_rbox.c)
    rbox_output == RboxPoints

notes:
    only called from qh_rboxpoints()
    same as fprintf() and Qhull::qh_fprintf()
    fgets() is not trapped like fprintf()
    Do not throw errors from here.  Use qh_errexit_rbox;
*/
extern "C"
void qh_fprintf_rbox(FILE*, int msgcode, const char *fmt, ... ) {
    va_list args;

    using namespace orgQhull;

    RboxPoints *out= rbox_output;
    va_start(args, fmt);
    if(msgcode<MSG_OUTPUT){
        char newMessage[MSG_MAXLEN];
        // RoadError provides the message tag
        vsnprintf(newMessage, sizeof(newMessage), fmt, args);
        out->rbox_message += newMessage;
        if(out->rbox_status<MSG_ERROR || out->rbox_status>=MSG_STDERR){
            out->rbox_status= msgcode;
        }
        va_end(args);
        return;
    }
    switch(msgcode){
    case 9391:
    case 9392:
        out->rbox_message += "RboxPoints error: options 'h', 'n' not supported.\n";
        qh_errexit_rbox(10010);
        /* never returns */
    case 9393:
        {
            int dimension= va_arg(args, int);
            string command(va_arg(args, char*));
            int count= va_arg(args, int);
            out->setDimension(dimension);
            out->appendComment(" \"");
            out->appendComment(command.substr(command.find(' ')+1));
            out->appendComment("\"");
            out->setNewCount(count);
            out->reservePoints();
        }
        break;
    case 9407:
        *out << va_arg(args, int);
        // fall through
    case 9405:
        *out << va_arg(args, int);
        // fall through
    case 9403:
        *out << va_arg(args, int);
        break;
    case 9408:
        *out << va_arg(args, double);
        // fall through
    case 9406:
        *out << va_arg(args, double);
        // fall through
    case 9404:
        *out << va_arg(args, double);
        break;
    }
    va_end(args);
} /* qh_fprintf_rbox */

