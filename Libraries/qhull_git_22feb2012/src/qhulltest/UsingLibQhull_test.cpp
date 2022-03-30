/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/qhulltest/UsingLibQhull_test.cpp#4 $$Change: 1490 $
** $DateTime: 2012/02/19 20:27:01 $$Author: bbarber $
**
****************************************************************************/

//pre-compiled headers
#include <iostream>
#include "RoadTest.h" // QT_VERSION

#include "UsingLibQhull.h"
#include "QhullError.h"
#include "Qhull.h"

using std::cout;
using std::endl;
using std::string;

namespace orgQhull {

//! Test C++ interface to Qhull
//! See eg/q_test for tests of Qhull commands
class UsingLibQhull_test : public RoadTest
{
    Q_OBJECT

#//Test slots
private slots:
    void cleanup();
    void t_classMembers();
    void t_globalPoints();
    void t_UsingLibQhull();
    void t_methods();
    void t_cleanuptestcase();
};//UsingLibQhull_test

void
add_UsingLibQhull_test()
{
    new UsingLibQhull_test();
}

//Executed after each testcase
void UsingLibQhull_test::
cleanup()
{
    UsingLibQhull::checkQhullMemoryEmpty();
    RoadTest::cleanup();
}

void UsingLibQhull_test::
t_classMembers()
{
    {
        //checkQhullMemoryEmpty tested by cleanup()
        QCOMPARE(UsingLibQhull::globalMachineEpsilon()+1.0, 1.0);
        RboxPoints r10("10");
        Qhull q(r10,"v");  // voronoi diagram of 10 points
        UsingLibQhull::unsetGlobalAngleEpsilon();
        UsingLibQhull::unsetGlobalDistanceEpsilon();
        cout << "MachineEpsilon " << UsingLibQhull::globalMachineEpsilon()
            << " angleEpsilon " << UsingLibQhull::globalAngleEpsilon()
            << " distanceEpsilon " << UsingLibQhull::globalDistanceEpsilon()
            << endl;
        QCOMPARE(UsingLibQhull::currentAngleEpsilon()+1.0, 1.0);
        QVERIFY(UsingLibQhull::currentAngleEpsilon() > UsingLibQhull::globalMachineEpsilon());
        QCOMPARE(UsingLibQhull::currentDistanceEpsilon()+1.0, 1.0);
        QVERIFY(UsingLibQhull::currentDistanceEpsilon() >= UsingLibQhull::currentAngleEpsilon());
        QCOMPARE(UsingLibQhull::currentQhull().runId(), q.runId());
        QCOMPARE(UsingLibQhull::globalAngleEpsilon()+1.0, UsingLibQhull::currentAngleEpsilon()+1.0);
        QCOMPARE(UsingLibQhull::currentVertexDimension(), q.dimension());
        QCOMPARE(UsingLibQhull::globalDistanceEpsilon()+1.0, UsingLibQhull::currentDistanceEpsilon()+1.0);
        UsingLibQhull::setGlobalAngleEpsilon(1.0);
        UsingLibQhull::setGlobalDistanceEpsilon(1.0);
        cout << " Global angleEpsilon " << UsingLibQhull::globalAngleEpsilon()
            << " distanceEpsilon " << UsingLibQhull::globalDistanceEpsilon()
            << endl;
        QCOMPARE(UsingLibQhull::globalAngleEpsilon(), UsingLibQhull::globalDistanceEpsilon());
        QVERIFY(UsingLibQhull::currentAngleEpsilon() != UsingLibQhull::globalAngleEpsilon());
        UsingLibQhull::setGlobalVertexDimension(3);
        QCOMPARE(UsingLibQhull::globalVertexDimension(), UsingLibQhull::currentVertexDimension());
        UsingLibQhull::setGlobalVertexDimension(2);
        QCOMPARE(UsingLibQhull::globalVertexDimension(), 2);
        QCOMPARE(UsingLibQhull::currentVertexDimension(), q.dimension());
        QVERIFY(UsingLibQhull::currentDistanceEpsilon() != UsingLibQhull::globalDistanceEpsilon());
        UsingLibQhull::unsetGlobalAngleEpsilon();
        UsingLibQhull::unsetGlobalVertexDimension();
        UsingLibQhull::unsetGlobalDistanceEpsilon();
        QCOMPARE(UsingLibQhull::currentAngleEpsilon()+1.0, UsingLibQhull::globalAngleEpsilon()+1.0);
        QCOMPARE(UsingLibQhull::globalVertexDimension(), UsingLibQhull::currentVertexDimension());
        QCOMPARE(UsingLibQhull::currentDistanceEpsilon()+1.0, UsingLibQhull::globalDistanceEpsilon()+1.0);
        UsingLibQhull::setGlobals();
    }
    QCOMPARE(UsingLibQhull::globalAngleEpsilon()+1.0, 1.0);
    QCOMPARE(UsingLibQhull::globalVertexDimension(), 4); // 'v'.  VertexDimension is only used for QhullVertex where dim>15
    QCOMPARE(UsingLibQhull::globalDistanceEpsilon()+1.0, 1.0);
    UsingLibQhull::unsetGlobals();
    try{
        cout << UsingLibQhull::globalVertexDimension();
        QFAIL("Did not throw error for undefined dimension.");
    }catch(const std::exception &e){
        cout << "INFO     Caught error -- " << e.what() << endl;
    }
}//t_classMembers

void UsingLibQhull_test::
t_globalPoints()
{
    const coordT *r10PointsBegin;
    {
        RboxPoints r10("10");
        Qhull q(r10,"v");  // voronoi diagram of 10 points
        UsingLibQhull::unsetGlobalPoints();
        int dimension;
        const coordT *pointsEnd;
        const coordT *pointsBegin= UsingLibQhull::globalPoints(&dimension, &pointsEnd);
        cout << "pointsBegin " << pointsBegin
            << " pointsEnd " << pointsEnd
            << " dimension " << dimension
            << endl;
        int dimension2;
        const coordT *pointsEnd2;
        const coordT *pointsBegin2= UsingLibQhull::currentPoints(&dimension2, &pointsEnd2);
        QCOMPARE(pointsBegin2, pointsBegin);
        QCOMPARE(pointsEnd2, pointsEnd);
        QCOMPARE(dimension2, dimension);
        coordT c[]= { 1.0,2.0, 3.0,4.0, 5.0,6.0 };
        UsingLibQhull::setGlobalPoints(2, c, c+3*2);
        pointsBegin= UsingLibQhull::globalPoints(&dimension, &pointsEnd);
        QCOMPARE(pointsBegin, c);
        QCOMPARE(pointsEnd[-1], 6.0);
        QCOMPARE(dimension, 2);
        UsingLibQhull::unsetGlobalPoints();
        pointsBegin= UsingLibQhull::globalPoints(&dimension, &pointsEnd);
        QCOMPARE(pointsBegin, pointsBegin2);
        QCOMPARE(pointsEnd, pointsEnd2);
        QCOMPARE(dimension, dimension2);
        UsingLibQhull::setGlobals();
        r10PointsBegin= pointsBegin;
    }
    int dimension3;
    const coordT *pointsEnd3;
    const coordT *pointsBegin3= UsingLibQhull::currentPoints(&dimension3, &pointsEnd3);
    QCOMPARE(pointsBegin3, r10PointsBegin); // Memory was freed
    QCOMPARE(pointsEnd3, r10PointsBegin+10*4);
    QCOMPARE(dimension3, 4);
    UsingLibQhull::unsetGlobals();
    try{
        pointsBegin3= UsingLibQhull::globalPoints(&dimension3, &pointsEnd3);
        QFAIL("Did not throw error for undefined global points.");
    }catch(const std::exception &e){
        cout << "INFO     Caught error -- " << e.what() << endl;
    }
}//t_globalPoints

void UsingLibQhull_test::
t_UsingLibQhull()
{
    {
        Qhull q;
        UsingLibQhull uq(&q); // Normally created in a method using 'this'

        try{
            Qhull q2; // If qh_QHpointer, QhullQh() calls usinlibqhull()
            UsingLibQhull uq2(&q2);
            QFAIL("UsingLibQhull did not fail.");
        }catch (const std::exception &e) {
            cout << "INFO   : Caught " << e.what();
        }
    }
    Qhull q3;
    UsingLibQhull uq3(&q3);
    // UsingLibQhull uq4; // Default constructors disabled.
}//t_UsingLibQhull

void UsingLibQhull_test::
t_methods()
{
    Qhull q;
    UsingLibQhull u(&q); // Normally created in a method using 'this'
    QVERIFY(u.defined());
    u.maybeThrowQhullMessage(0);  // Nothing thrown
    try{
        u.maybeThrowQhullMessage(1);
        QFAIL("maybeThrowQhullMessage(1) did not fail.");
    }catch (const std::exception &e) {
        cout << "INFO   : Caught " << e.what();
    }
    // Can not check checkRunId() in maybeThrowQhullMessage().  Requires another thread.
    u.maybeThrowQhullMessage(2, UsingLibQhull::NOthrow);
    try{
        throw QhullError(10054, "Report previous NOthrow error");
    }catch (const std::exception &e) {
        cout << "INFO   : " << e.what();
    }
}//t_methods

// Executed after last test
void UsingLibQhull_test::
t_cleanuptestcase()
{
    UsingLibQhull::unsetGlobals();
}//t_cleanuptestcase

}//orgQhull

#include "moc/UsingLibQhull_test.moc"

