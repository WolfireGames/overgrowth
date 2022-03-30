/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/libqhullcpp/UsingLibQhull.h#6 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#ifndef USINGlibqhull_H
#define USINGlibqhull_H

#include "QhullError.h"
extern "C" {
#include "libqhull/libqhull.h"
}

namespace orgQhull {

#//Types
    //! UsingLibQhull -- Interface into libqhull and its 'qh' and 'qhstat' macros
    //! Always use with setjmp() for libqhull error handling.

/*******************************

UsingLibQhull is stack based, but as a call
Qhull declarations are stack-based.  But can't define a
setjmp environment, since the target goes away.  So must be UsingLibQhull, but can only have one
setjmp at a time? Can embedded another Using as long as save/restore
longjmp on exit.
*/
    class UsingLibQhull;

    // Defined elsewhere
    class Qhull;

#// Global variables
extern Qhull           *s_qhull_output; //! Provide qh_fprintf (Qhull.cpp) access to Qhull

class UsingLibQhull {

private:
#//Fields
    Qhull              *my_qhull;
    int                 qh_exitcode;

#//Class globals
    //! Global flags
    static bool         s_using_libqhull; //! True if UsingLibQhull is in scope

    //! Use global values if s_has_... is set
    static bool         s_has_angle_epsilon; //! True if s_angle_epsilon defined
    static bool         s_has_distance_epsilon; //! True if s_distance_epsilon defined
    static bool         s_has_points;        //! If False (default), Qhull() runs setPointBase()
    static bool         s_has_vertex_dimension; //! True if s_vertex_dimension defined

    //! Global values
    static double       s_angle_epsilon;   //! Epsilon for angle equality
    static double       s_distance_epsilon;   //! Epsilon for distance equality
    static const coordT *s_points_begin;            //! For QhullPoint::id() w/o qhRunId.
    static const coordT *s_points_end;            //! For QhullPoint::id() w/o qhRunId.
    static int          s_points_dimension;
    static int          s_vertex_dimension; //! Default dimension (e.g., if Vertex::dimension() >= 16)

public:
#//Class constants
    static const int    NOqhRunId= 0;   //! qh_qh is not available
    static const int    NOthrow= 1;     //! Do not throw from maybeThrowQhullMessage
    static const int    FACTORepsilon= 10;  //!
    static const double DEFAULTdistanceEpsilon; //! ~DISTround*FACTORepsilon for unit cube
    static const double DEFAULTangleEpsilon;    //! ~ANGLEround*FACTORepsilon for unit cube

#//Class members
    static void         checkQhullMemoryEmpty();
    static double       currentAngleEpsilon();
    static double       currentDistanceEpsilon();
    static const coordT *currentPoints(int *dimension, const coordT **pointsEnd);
    static Qhull       &currentQhull();
    static int          currentVertexDimension();
    static double       globalAngleEpsilon() { return s_has_angle_epsilon ? s_angle_epsilon : currentAngleEpsilon(); }
    static double       globalDistanceEpsilon() { return s_has_distance_epsilon ? s_distance_epsilon : currentDistanceEpsilon(); }
    static double       globalMachineEpsilon() { return REALepsilon; }
    static const coordT *globalPoints(int *dimension, const coordT **pointsEnd);
    static int          globalVertexDimension() { return s_has_vertex_dimension ? s_vertex_dimension : currentVertexDimension(); }
    static bool         hasPoints();        // inline would require Qhull.h
    static bool         hasVertexDimension();
    static void         setGlobalAngleEpsilon(double d) { s_angle_epsilon=d; s_has_angle_epsilon= true; }
    static void         setGlobalDistanceEpsilon(double d) { s_distance_epsilon= d; s_has_distance_epsilon= true; }
    static void         setGlobalPoints(int dimension, const coordT *pointsBegin, const coordT *pointsEnd) { s_points_dimension= dimension; s_points_begin= pointsBegin; s_points_end= pointsEnd; s_has_points= true; }
    static void         setGlobalVertexDimension(int i) { s_vertex_dimension= i; s_has_vertex_dimension= true; }
    static void         setGlobals();
    static void         unsetGlobalAngleEpsilon() { s_has_angle_epsilon= false; }
    static void         unsetGlobalDistanceEpsilon() { s_has_distance_epsilon= false; }
    static void         unsetGlobalPoints() { s_has_points= false; }
    static void         unsetGlobalVertexDimension() { s_has_vertex_dimension= false; }
    static void         unsetGlobals();

#//Constructors
                        UsingLibQhull(Qhull *p);
                        UsingLibQhull(Qhull *p, int noThrow);
                        UsingLibQhull(int qhRunId);
                       ~UsingLibQhull();

private:                //! disable default constructor, copy constructor, and copy assignment
                        UsingLibQhull();
                        UsingLibQhull(const UsingLibQhull &);
   UsingLibQhull       &operator=(const UsingLibQhull &);
public:

#//Methods
#//Access
    bool                defined() const { return my_qhull!=0; }
    void                maybeThrowQhullMessage(int exitCode) const;
    void                maybeThrowQhullMessage(int exitCode, int noThrow) const;

#//Helpers
private:
   QhullError           checkRunId() const;
   void                 checkUsingLibQhull() const;

/***********************************
You may use global variables in 'qh' after declaring UsingLibQhull.  For example

  UsingLibQhull q(qhRunId);
  // NOerrors -- no calls that throw libqhull errors
  cout << "Delaunay Mode: " << qh DELAUNAY;

To trap errors from libqhull, UsingLibQhull must be followed by

UsingLibQhull q(qhRunId);
int exitCode = setjmp(qh errexit);
if(!exitCode){ // no object creation -- destructors skipped on longjmp()
    calls to libqhull
}
q.maybeThrowQhullMessage(exitCode);

The call to setjmp() can not be moved to a method.  The stack must be preserved for error exits from libqhull.

*/

};//UsingLibQhull

}//namespace orgQhull

#endif // USINGlibqhull_H
