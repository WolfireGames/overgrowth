/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/RoadError.cpp#1 $$Change: 1490 $
** $DateTime: 2012/02/19 20:27:01 $$Author: bbarber $
**
****************************************************************************/

#//! RoadError -- All exceptions thrown by Qhull are RoadErrors
#//! Do not throw RoadError's from destructors.  Use e.logError() instead.

#include "RoadError.h"

#include <string>
#include <sstream>
#include <iostream>

using std::cerr;
using std::cout;
using std::string;

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#endif

namespace orgQhull {

#//Class fields

//! Identifies error messages from Qhull and Road for web searches.
//! See QhullError.h#QHULLlastError and user.h#MSG_ERROR
const char * RoadError::
ROADtag= "QH";

std::ostringstream RoadError::
global_log;

#//Constructor

RoadError::
RoadError()
: error_code(0)
, log_event()
, error_message()
{ }

RoadError::
RoadError(const RoadError &other)
: error_code(other.error_code)
, log_event(other.log_event)
, error_message(other.error_message)
{
}//copy construct

RoadError::
RoadError(int code, const std::string &message)
: error_code(code)
, log_event(message.c_str())
, error_message(log_event.toString(ROADtag, error_code))
{
    log_event.cstr_1= error_message.c_str(); // overwrites initial value
}

RoadError::
RoadError(int code, const char *fmt)
: error_code(code)
, log_event(fmt)
, error_message()
{ }

RoadError::
RoadError(int code, const char *fmt, int d)
: error_code(code)
, log_event(fmt, d)
, error_message()
{ }

RoadError::
RoadError(int code, const char *fmt, int d, int d2)
: error_code(code)
, log_event(fmt, d, d2)
, error_message()
{ }

RoadError::
RoadError(int code, const char *fmt, int d, int d2, float f)
: error_code(code)
, log_event(fmt, d, d2, f)
, error_message()
{ }

RoadError::
RoadError(int code, const char *fmt, int d, int d2, float f, const char *s)
: error_code(code)
, log_event(fmt, d, d2, f, s)
, error_message(log_event.toString(ROADtag, code)) // char * may go out of scope
{ }

RoadError::
RoadError(int code, const char *fmt, int d, int d2, float f, const void *x)
: error_code(code)
, log_event(fmt, d, d2, f, x)
, error_message()
{ }

RoadError::
RoadError(int code, const char *fmt, int d, int d2, float f, int i)
: error_code(code)
, log_event(fmt, d, d2, f, i)
, error_message()
{ }

RoadError::
RoadError(int code, const char *fmt, int d, int d2, float f, long long i)
: error_code(code)
, log_event(fmt, d, d2, f, i)
, error_message()
{ }

RoadError::
RoadError(int code, const char *fmt, int d, int d2, float f, double e)
: error_code(code)
, log_event(fmt, d, d2, f, e)
, error_message()
{ }

RoadError & RoadError::
operator=(const RoadError &other)
{
    error_code= other.error_code;
    error_message= other.error_message;
    log_event= other.log_event;
    return *this;
}//operator=

#//Virtual
const char * RoadError::
what() const throw()
{
    if(error_message.empty()){
        error_message= log_event.toString(ROADtag, error_code);
    }
    return error_message.c_str();
}//what

#//Updates

//! Log error instead of throwing it.
void RoadError::
logError() const
{
    global_log << what() << endl;
}//logError


}//namespace orgQhull

