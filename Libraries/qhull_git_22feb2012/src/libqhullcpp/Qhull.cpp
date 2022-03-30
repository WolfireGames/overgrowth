/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/Qhull.cpp#6 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#//! Qhull -- invoke qhull from C++
#//! Compile libqhull and Qhull together due to use of setjmp/longjmp()

#include "QhullError.h"
#include "UsingLibQhull.h"
#include "RboxPoints.h"
#include "QhullQh.h"
#include "QhullFacet.h"
#include "QhullFacetList.h"
#include "Qhull.h"
extern "C" {
    #include "libqhull/qhull_a.h"
}

#include <iostream>

using std::cerr;
using std::string;
using std::vector;
using std::ostream;

#ifdef _MSC_VER  // Microsoft Visual C++ -- warning level 4
#pragma warning( disable : 4611)  // interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning( disable : 4996)  // function was declared deprecated(strcpy, localtime, etc.)
#endif

namespace orgQhull {

#//Global variables

char s_unsupported_options[]=" Fd TI ";
char s_not_output_options[]= " Fd TI A C d E H P Qb QbB Qbb Qc Qf Qg Qi Qm QJ Qr QR Qs Qt Qv Qx Qz Q0 Q1 Q2 Q3 Q4 Q5 Q6 Q7 Q8 Q9 Q10 Q11 R Tc TC TM TP TR Tv TV TW U v V W ";

#//Constructor, destructor, etc.
Qhull::
Qhull()
: qhull_qh(0)
, qhull_run_id(UsingLibQhull::NOqhRunId)
, origin_point()
, qhull_status(qh_ERRnone)
, qhull_dimension(0)
, run_called(false)
, qh_active(false)
, qhull_message()
, error_stream(0)
, output_stream(0)
, feasiblePoint()
, useOutputStream(false)
{
    initializeQhull();
}//Qhull

Qhull::
Qhull(const RboxPoints &rboxPoints, const char *qhullCommand2)
: qhull_qh(0)
, qhull_run_id(UsingLibQhull::NOqhRunId)
, origin_point()
, qhull_status(qh_ERRnone)
, qhull_dimension(0)
, run_called(false)
, qh_active(false)
, qhull_message()
, error_stream(0)
, output_stream(0)
, feasiblePoint()
, useOutputStream(false)
{
    initializeQhull();
    runQhull(rboxPoints, qhullCommand2);
}//Qhull rbox

Qhull::
Qhull(const char *rboxCommand2, int pointDimension, int pointCount, const realT *pointCoordinates, const char *qhullCommand2)
: qhull_qh(0)
, qhull_run_id(UsingLibQhull::NOqhRunId)
, origin_point()
, qhull_status(qh_ERRnone)
, qhull_dimension(0)
, run_called(false)
, qh_active(false)
, qhull_message()
, error_stream(0)
, output_stream(0)
, feasiblePoint()
, useOutputStream(false)
{
    initializeQhull();
    runQhull(rboxCommand2, pointDimension, pointCount, pointCoordinates, qhullCommand2);
}//Qhull points

Qhull::
Qhull(const Qhull &other)
: qhull_qh(0)
, qhull_run_id(UsingLibQhull::NOqhRunId)
, origin_point()
, qhull_status(qh_ERRnone)
, qhull_dimension(0)
, run_called(false)
, qh_active(false)
, qhull_message(other.qhull_message)
, error_stream(other.error_stream)
, output_stream(other.output_stream)
, feasiblePoint(other.feasiblePoint)
, useOutputStream(other.useOutputStream)
{
    if(other.initialized()){
        throw QhullError(10069, "Qhull error: can not use Qhull copy constructor if initialized() is true");
    }
    initializeQhull();
}//copy constructor

Qhull & Qhull::
operator=(const Qhull &other)
{
    if(other.initialized() || initialized()){
        throw QhullError(10070, "Qhull error: can not use Qhull copy assignment if initialized() is true");
    }
    qhull_message= other.qhull_message;
    error_stream= other.error_stream;
    output_stream= other.output_stream;
    feasiblePoint= other.feasiblePoint;
    useOutputStream= other.useOutputStream;
    return *this;
}//copy constructor

void Qhull::
initializeQhull()
{
    #if qh_QHpointer
        qhull_qh= new QhullQh;
        qhull_qh->old_qhstat= qh_qhstat;
        qhull_qh->old_tempstack= qhmem.tempstack;
        qh_qh= 0;
        qh_qhstat= 0;
    #else
        qhull_qh= new (&qh_qh) QhullQh;
        qhull_qh->old_qhstat= &qh_qhstat;
        qhull_qh->old_tempstack= qhmem.tempstack;
    #endif
    qhmem.tempstack= 0;
    qhull_run_id= qhull_qh->run_id;
}//initializeQhull

Qhull::
~Qhull() throw()
{
    //! UsingLibQhull is required by ~QhullQh
    UsingLibQhull q(this, QhullError::NOthrow);
    if(q.defined()){
        int exitCode = setjmp(qh errexit);
        if(!exitCode){ // no object creation -- destructors skipped on longjmp()
#if qh_QHpointer
            delete qhull_qh;
            // clears qhull_qh and qh_qh
            qh_qh= 0;
#else
            qhull_qh->~QhullQh();
            qhull_qh= 0;
#endif
            qhull_run_id= UsingLibQhull::NOqhRunId;
            // Except for cerr, does not throw errors
            if(hasQhullMessage()){
                cerr<< "\nQhull output at end\n"; //FIXUP QH11005: where should error and log messages go on ~Qhull?
                cerr<<qhullMessage();
                clearQhullMessage();
            }
        }
        maybeThrowQhullMessage(exitCode, QhullError::NOthrow);
    }
    s_qhull_output= 0; // Set by UsingLibQhull
}//~Qhull

#//Messaging

void Qhull::
appendQhullMessage(const string &s)
{
    if(output_stream && useOutputStream && qh USEstdout){   // threading errors caught elsewhere
        *output_stream << s;
    }else if(error_stream){
        *error_stream << s;
    }else{
        qhull_message += s;
    }
}//appendQhullMessage

//! clearQhullMessage does not throw errors (~Qhull)
void Qhull::
clearQhullMessage()
{
    qhull_status= qh_ERRnone;
    qhull_message.clear();
    RoadError::clearGlobalLog();
}//clearQhullMessage

//! hasQhullMessage does not throw errors (~Qhull)
bool Qhull::
hasQhullMessage() const
{
    return (!qhull_message.empty() || qhull_status!=qh_ERRnone);
    //FIXUP QH11006 -- inconsistent usage with Rbox.  hasRboxMessage just tests rbox_status.  No appendRboxMessage()
}

//! qhullMessage does not throw errors (~Qhull)
std::string Qhull::
qhullMessage() const
{
    if(qhull_message.empty() && qhull_status!=qh_ERRnone){
        return "qhull: no message for error.  Check cerr or error stream\n";
    }else{
        return qhull_message;
    }
}//qhullMessage

int Qhull::
qhullStatus() const
{
    return qhull_status;
}//qhullStatus

void Qhull::
setErrorStream(ostream *os)
{
    error_stream= os;
}//setErrorStream

//! Updates useOutputStream
void Qhull::
setOutputStream(ostream *os)
{
    output_stream= os;
    useOutputStream= (os!=0);
}//setOutputStream

#//GetSet

void Qhull::
checkIfQhullInitialized()
{
    if(!initialized()){ // qh_initqhull_buffers() not called
        throw QhullError(10023, "Qhull error: checkIfQhullInitialized failed.  Call runQhull() first.");
    }
}//checkIfQhullInitialized

//! Setup global state (qh_qh, qh_qhstat, qhmem.tempstack)
int Qhull::
runId()
{
    UsingLibQhull u(this);
    QHULL_UNUSED(u);

    return qhull_run_id;
}//runId


#//GetValue

double Qhull::
area(){
    checkIfQhullInitialized();
    UsingLibQhull q(this);
    if(!qh hasAreaVolume){
        int exitCode = setjmp(qh errexit);
        if(!exitCode){ // no object creation -- destructors skipped on longjmp()
            qh_getarea(qh facet_list);
        }
        maybeThrowQhullMessage(exitCode);
    }
    return qh totarea;
}//area

double Qhull::
volume(){
    checkIfQhullInitialized();
    UsingLibQhull q(this);
    if(!qh hasAreaVolume){
        int exitCode = setjmp(qh errexit);
        if(!exitCode){ // no object creation -- destructors skipped on longjmp()
            qh_getarea(qh facet_list);
        }
        maybeThrowQhullMessage(exitCode);
    }
    return qh totvol;
}//volume

#//ForEach

//! Define QhullVertex::neighborFacets().
//! Automatically called if merging facets or computing the Voronoi diagram.
//! Noop if called multiple times.
void Qhull::
defineVertexNeighborFacets(){
    checkIfQhullInitialized();
    UsingLibQhull q(this);
    if(!qh hasAreaVolume){
        int exitCode = setjmp(qh errexit);
        if(!exitCode){ // no object creation -- destructors skipped on longjmp()
            qh_vertexneighbors();
        }
        maybeThrowQhullMessage(exitCode);
    }
}//defineVertexNeighborFacets

QhullFacetList Qhull::
facetList() const{
    return QhullFacetList(beginFacet(), endFacet());
}//facetList

QhullPoints Qhull::
points() const
{
    return QhullPoints(hullDimension(), qhull_qh->num_points*hullDimension(), qhull_qh->first_point);
}//points

QhullPointSet Qhull::
otherPoints() const
{
    return QhullPointSet(hullDimension(), qhull_qh->other_points);
}//otherPoints

//! Return vertices of the convex hull.
QhullVertexList Qhull::
vertexList() const{
    return QhullVertexList(beginVertex(), endVertex());
}//vertexList

#//Modify

void Qhull::
outputQhull()
{
    checkIfQhullInitialized();
    UsingLibQhull q(this);
    int exitCode = setjmp(qh errexit);
    if(!exitCode){ // no object creation -- destructors skipped on longjmp()
        qh_produce_output2();
    }
    maybeThrowQhullMessage(exitCode);
}//outputQhull

void Qhull::
outputQhull(const char *outputflags)
{
    checkIfQhullInitialized();
    UsingLibQhull q(this);
    string cmd(" "); // qh_checkflags skips first word
    cmd += outputflags;
    char *command= const_cast<char*>(cmd.c_str());
    int exitCode = setjmp(qh errexit);
    if(!exitCode){ // no object creation -- destructors skipped on longjmp()
        qh_clear_outputflags();
        char *s = qh qhull_command + strlen(qh qhull_command) + 1; //space
        strncat(qh qhull_command, command, sizeof(qh qhull_command)-strlen(qh qhull_command)-1);
        qh_checkflags(command, s_not_output_options);
        qh_initflags(s);
        qh_initqhull_outputflags();
        if(qh KEEPminArea < REALmax/2
           || (0 != qh KEEParea + qh KEEPmerge + qh GOODvertex
                    + qh GOODthreshold + qh GOODpoint + qh SPLITthresholds)){
            facetT *facet;
            qh ONLYgood= False;
            FORALLfacet_(qh facet_list) {
                facet->good= True;
            }
            qh_prepare_output();
        }
        qh_produce_output2();
        if(qh VERIFYoutput && !qh STOPpoint && !qh STOPcone){
            qh_check_points();
        }
    }
    maybeThrowQhullMessage(exitCode);
}//outputQhull

void Qhull::
runQhull(const RboxPoints &rboxPoints, const char *qhullCommand2)
{
    runQhull(rboxPoints.comment().c_str(), rboxPoints.dimension(), rboxPoints.count(), &*rboxPoints.coordinates(), qhullCommand2);
}//runQhull, RboxPoints

//! points is a array of points, input sites ('d' or 'v'), or halfspaces with offset last ('H')
//! Derived from qh_new_qhull [user.c]
void Qhull::
runQhull(const char *rboxCommand2, int pointDimension, int pointCount, const realT *rboxPoints, const char *qhullCommand2)
{
    if(run_called){
        throw QhullError(10027, "Qhull error: runQhull called twice.  Only one call allowed.");
    }
    run_called= true;
    string s("qhull ");
    s += qhullCommand2;
    char *command= const_cast<char*>(s.c_str());
    UsingLibQhull q(this);
    int exitCode = setjmp(qh errexit);
    if(!exitCode){ // no object creation -- destructors skipped on longjmp()
        qh_checkflags(command, s_unsupported_options);
        qh_initflags(command);
        *qh rbox_command= '\0';
        strncat( qh rbox_command, rboxCommand2, sizeof(qh rbox_command)-1);
        if(qh DELAUNAY){
            qh PROJECTdelaunay= True;   // qh_init_B() calls qh_projectinput()
        }
        pointT *newPoints= const_cast<pointT*>(rboxPoints);
        int newDimension= pointDimension;
        int newIsMalloc= False;
        if(qh HALFspace){
            --newDimension;
            initializeFeasiblePoint(newDimension);
            newPoints= qh_sethalfspace_all(pointDimension, pointCount, newPoints, qh feasible_point);
            newIsMalloc= True;
        }
        qh_init_B(newPoints, pointCount, newDimension, newIsMalloc);
        qhull_dimension= (qh DELAUNAY ? qh hull_dim - 1 : qh hull_dim);
        qh_qhull();
        qh_check_output();
        qh_prepare_output();
        if(qh VERIFYoutput && !qh STOPpoint && !qh STOPcone){
            qh_check_points();
        }
    }
    for(int k= qhull_dimension; k--; ){  // Do not move up (may throw)
        origin_point << 0.0;
    }
    maybeThrowQhullMessage(exitCode);
}//runQhull

#//Helpers -- be careful of allocating C++ objects due to setjmp/longjmp() error handling by qh_... routines

void Qhull::
initializeFeasiblePoint(int hulldim)
{
    if(qh feasible_string){
        qh_setfeasible(hulldim);
    }else{
        if(feasiblePoint.empty()){
            qh_fprintf(qh ferr, 6209, "qhull error: missing feasible point for halfspace intersection.  Use option 'Hn,n' or set qh.feasiblePoint\n");
            qh_errexit(qh_ERRmem, NULL, NULL);
        }
        if(feasiblePoint.size()!=(size_t)hulldim){
            qh_fprintf(qh ferr, 6210, "qhull error: dimension of feasiblePoint should be %d.  It is %u", hulldim, feasiblePoint.size());
            qh_errexit(qh_ERRmem, NULL, NULL);
        }
        if (!(qh feasible_point= (coordT*)qh_malloc(hulldim * sizeof(coordT)))) {
            qh_fprintf(qh ferr, 6202, "qhull error: insufficient memory for feasible point\n");
            qh_errexit(qh_ERRmem, NULL, NULL);
        }
        coordT *t= qh feasible_point;
        // No qh_... routines after here -- longjmp() ignores destructor
        for(Coordinates::ConstIterator p=feasiblePoint.begin(); p<feasiblePoint.end(); p++){
            *t++= *p;
        }
    }
}//initializeFeasiblePoint

void Qhull::
maybeThrowQhullMessage(int exitCode)
{
    if(qhull_status==qh_ERRnone){
        qhull_status= exitCode;
    }
    if(qhull_status!=qh_ERRnone){
        QhullError e(qhull_status, qhull_message);
        clearQhullMessage();
        throw e; // FIXUP QH11007: copy constructor is expensive if logging
    }
}//maybeThrowQhullMessage

void Qhull::
maybeThrowQhullMessage(int exitCode, int noThrow)  throw()
{
    QHULL_UNUSED(noThrow);

    if(qhull_status==qh_ERRnone){
        qhull_status= exitCode;
    }
    if(qhull_status!=qh_ERRnone){
        QhullError e(qhull_status, qhull_message);
        e.logError();
    }
}//maybeThrowQhullMessage

}//namespace orgQhull

/*-<a                             href="qh-user.htm#TOC"
 >-------------------------------</a><a name="qh_fprintf">-</a>

  qh_fprintf(fp, msgcode, format, list of args )
    fp is ignored (replaces qh_fprintf() in userprintf.c)
    s_qhull_output == Qhull

notes:
    only called from libqhull
    same as fprintf() and RboxPoints::qh_fprintf_rbox()
    fgets() is not trapped like fprintf()
    Do not throw errors from here.  Use qh_errexit;
*/
extern "C"
void qh_fprintf(FILE *fp, int msgcode, const char *fmt, ... ) {
    va_list args;

    using namespace orgQhull;

    if(!s_qhull_output){
        fprintf(stderr, "QH10025 Qhull error: UsingLibQhull not declared prior to calling qh_...().  s_qhull_output==NULL.\n");
        qh_exit(10025);
    }
    Qhull *out= s_qhull_output;
    va_start(args, fmt);
    if(msgcode<MSG_OUTPUT || fp == qh_FILEstderr){
        if(msgcode>=MSG_ERROR && msgcode<MSG_WARNING){
            if(out->qhull_status<MSG_ERROR || out->qhull_status>=MSG_WARNING){
                out->qhull_status= msgcode;
            }
        }
        char newMessage[MSG_MAXLEN];
        // RoadError will add the message tag
        vsnprintf(newMessage, sizeof(newMessage), fmt, args);
        out->appendQhullMessage(newMessage);
        va_end(args);
        return;
    }
    if(out->output_stream && out->useOutputStream){
        char newMessage[MSG_MAXLEN];
        vsnprintf(newMessage, sizeof(newMessage), fmt, args);
        *out->output_stream << newMessage;
        va_end(args);
        return;
    }
    // FIXUP QH11008: how do users trap messages and handle input?  A callback?
    char newMessage[MSG_MAXLEN];
    vsnprintf(newMessage, sizeof(newMessage), fmt, args);
    out->appendQhullMessage(newMessage);
    va_end(args);
} /* qh_fprintf */

