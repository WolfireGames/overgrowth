/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/qhulltest/QhullRidge_test.cpp#5 $$Change: 1490 $
** $DateTime: 2012/02/19 20:27:01 $$Author: bbarber $
**
****************************************************************************/

//pre-compiled headers
#include <iostream>
#include "RoadTest.h"

#include "QhullRidge.h"
#include "QhullError.h"
#include "RboxPoints.h"
#include "QhullFacet.h"
#include "Qhull.h"

using std::cout;
using std::endl;
using std::ostringstream;
using std::ostream;
using std::string;

namespace orgQhull {

class QhullRidge_test : public RoadTest
{
    Q_OBJECT

#//Test slots
private slots:
    void cleanup();
    void t_construct();
    void t_getSet();
    void t_foreach();
    void t_io();
};//QhullRidge_test

void
add_QhullRidge_test()
{
    new QhullRidge_test();
}

//Executed after each testcase
void QhullRidge_test::
cleanup()
{
    UsingLibQhull::checkQhullMemoryEmpty();
    RoadTest::cleanup();
}

void QhullRidge_test::
t_construct()
{
    // Qhull.runQhull() constructs QhullFacets as facetT
    QhullRidge r;
    QVERIFY(!r.isDefined());
    QCOMPARE(r.dimension(),0);
    RboxPoints rcube("c");
    Qhull q(rcube,"QR0");  // triangulation of rotated unit cube
    QhullFacet f(q.firstFacet());
    QhullRidgeSet rs(f.ridges());
    QVERIFY(!rs.isEmpty()); // Simplicial facets do not have ridges()
    QhullRidge r2(rs.first());
    QCOMPARE(r2.dimension(), 2); // One dimension lower than the facet
    r= r2;
    QVERIFY(r.isDefined());
    QCOMPARE(r.dimension(), 2);
    QhullRidge r3= r2.getRidgeT();
    QCOMPARE(r,r3);
    QhullRidge r4= r2.getBaseT();
    QCOMPARE(r,r4);
    QhullRidge r5= r2; // copy constructor
    QVERIFY(r5==r2);
    QVERIFY(r5==r);
}//t_construct

void QhullRidge_test::
t_getSet()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube,"QR0");  // triangulation of rotated unit cube
        QCOMPARE(q.facetCount(), 6);
        QCOMPARE(q.vertexCount(), 8);
        QhullFacet f(q.firstFacet());
        QhullRidgeSet rs= f.ridges();
        QhullRidgeSetIterator i(rs);
        while(i.hasNext()){
            const QhullRidge r= i.next();
            cout << r.id() << endl;
            QVERIFY(r.bottomFacet()!=r.topFacet());
            QCOMPARE(r.dimension(), 2); // Ridge one-dimension less than facet
            QVERIFY(r.id()>=0 && r.id()<9*27);
            QVERIFY(r.isDefined());
            QVERIFY(r==r);
            QVERIFY(r==i.peekPrevious());
            QCOMPARE(r.otherFacet(r.bottomFacet()),r.topFacet());
            QCOMPARE(r.otherFacet(r.topFacet()),r.bottomFacet());
        }
        QhullRidgeSetIterator i2(i);
        QEXPECT_FAIL("", "SetIterator copy constructor not reset to BOT", Continue);
        QVERIFY(!i2.hasPrevious());
    }
}//t_getSet

void QhullRidge_test::
t_foreach()
{
    RboxPoints rcube("c");  // cube
    {
        Qhull q(rcube, "QR0"); // rotated cube
        QhullFacet f(q.firstFacet());
        foreach (QhullRidge r, f.ridges()){  // Qt only
            QhullVertexSet vs= r.vertices();
            QCOMPARE(vs.count(), 2);
            foreach (QhullVertex v, vs){  // Qt only
                QVERIFY(f.vertices().contains(v));
            }
        }
        QhullRidgeSet rs= f.ridges();
        QhullRidge r= rs.first();
        QhullRidge r2= r;
        QList<QhullVertex> vs;
        int count= 0;
        while(!count || r2!=r){
            ++count;
            QhullVertex v;
            QVERIFY2(r2.hasNextRidge3d(f),"A cube should only have non-simplicial facets.");
            QhullRidge r3= r2.nextRidge3d(f, &v);
            QVERIFY(!vs.contains(v));
            vs << v;
            r2= r2.nextRidge3d(f);
            QCOMPARE(r3, r2);
        }
        QCOMPARE(vs.count(), rs.count());
        QCOMPARE(count, rs.count());
    }
}//t_foreach

void QhullRidge_test::
t_io()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube, "");
        QhullFacet f(q.firstFacet());
        QhullRidgeSet rs= f.ridges();
        QhullRidge r= rs.first();
        ostringstream os;
        os << "Ridges Without runId\n" << rs << "Ridge\n" << r;
        os << "Ridge with runId\n" << r.print(q.runId());
        cout << os.str();
        QString s= QString::fromStdString(os.str());
        QCOMPARE(s.count(" r"), 6+2);
    }
}//t_io

}//orgQhull

#include "moc/QhullRidge_test.moc"
