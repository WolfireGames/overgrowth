/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/qhulltest/QhullVertex_test.cpp#6 $$Change: 1490 $
** $DateTime: 2012/02/19 20:27:01 $$Author: bbarber $
**
****************************************************************************/
//pre-compiled headers
#include <iostream>
#include "RoadTest.h"

#include "QhullVertex.h"
#include "Coordinates.h"
#include "QhullError.h"
#include "RboxPoints.h"
#include "QhullFacet.h"
#include "QhullFacetSet.h"
#include "QhullVertexSet.h"
#include "Qhull.h"

using std::cout;
using std::endl;
using std::ostringstream;
using std::ostream;
using std::string;

namespace orgQhull {

class QhullVertex_test : public RoadTest
{
    Q_OBJECT

#//Test slots
private slots:
    void cleanup();
    void t_constructConvert();
    void t_getSet();
    void t_foreach();
    void t_io();
};//QhullVertex_test

void
add_QhullVertex_test()
{
    new QhullVertex_test();
}

//Executed after each testcase
void QhullVertex_test::
cleanup()
{
    UsingLibQhull::checkQhullMemoryEmpty();
    RoadTest::cleanup();
}

void QhullVertex_test::
t_constructConvert()
{
    // Qhull.runQhull() constructs QhullFacets as facetT
    QhullVertex v;
    QVERIFY(!v.isDefined());
    QCOMPARE(v.dimension(),0);
    RboxPoints rcube("c");
    Qhull q(rcube,"Qt QR0");  // triangulation of rotated unit cube
    QhullVertex v2(q.beginVertex());
    QCOMPARE(v2.dimension(),3);
    v= v2;  // copy assignment
    QVERIFY(v.isDefined());
    QCOMPARE(v.dimension(),3);
    QhullVertex v5= v2; // copy constructor
    QVERIFY(v5==v2);
    QVERIFY(v5==v);
    QhullVertex v3= v2.getVertexT();
    QCOMPARE(v,v3);
    QhullVertex v4= v2.getBaseT();
    QCOMPARE(v,v4);
}//t_constructConvert

void QhullVertex_test::
t_getSet()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube,"Qt QR0");  // triangulation of rotated unit cube
        QCOMPARE(q.facetCount(), 12);
        QCOMPARE(q.vertexCount(), 8);

        // Also spot-test QhullVertexList.  See QhullLinkedList_test.cpp
        QhullVertexList vs= q.vertexList();
        QhullVertexListIterator i(vs);
        while(i.hasNext()){
            const QhullVertex v= i.next();
            cout << v.id() << endl;
            QCOMPARE(v.dimension(),3);
            QVERIFY(v.id()>=0 && v.id()<9);
            QVERIFY(v.isDefined());
            if(i.hasNext()){
                QCOMPARE(v.next(), i.peekNext());
                QVERIFY(v.next()!=v);
                QVERIFY(v.next().previous()==v);
            }
            QVERIFY(i.hasPrevious());
            QCOMPARE(v, i.peekPrevious());
        }
        QhullVertexListIterator i2(i);
        QEXPECT_FAIL("", "ListIterator copy constructor not reset to BOT", Continue);
        QVERIFY(!i2.hasPrevious());

        // test point()
        foreach (QhullVertex v, q.vertexList()){  // Qt only
            QhullPoint p= v.point();
            int j= p.id(q.runId());
            cout << "Point " << j << ":\n" << p.print(q.runId()) << endl;
            QVERIFY(j>=0 && j<8);
        }
    }
}//t_getSet

void QhullVertex_test::
t_foreach()
{
    RboxPoints rcube("c W0 300");  // 300 points on surface of cube
    {
        Qhull q(rcube, "QR0 Qc"); // keep coplanars, thick facet, and rotate the cube
        foreach (QhullVertex v, q.vertexList()){  // Qt only
            QhullFacetSet fs= v.neighborFacets();
            QCOMPARE(fs.count(), 3);
            foreach (QhullFacet f, fs){  // Qt only
                QVERIFY(f.vertices().contains(v));
            }
        }
    }
}//t_foreach

void QhullVertex_test::
t_io()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube, "");
        QhullVertex v= q.beginVertex();
        ostringstream os;
        os << "Vertex and vertices w/o runId:\n";
        os << v;
        QhullVertexSet vs= q.firstFacet().vertices();
        os << vs;
        os << "Vertex and vertices w/ runId:\n";
        os << v.print(q.runId());
        os << vs.print(q.runId(), "vertices:");
        cout << os.str();
        QString s= QString::fromStdString(os.str());
        QCOMPARE(s.count("(v"), 10);
        QCOMPARE(s.count(": f"), 2);
    }
    RboxPoints r10("10 D3");  // Without QhullVertex::facetNeighbors
    {
        Qhull q(r10, "");
        QhullVertex v= q.beginVertex();
        ostringstream os;
        os << "\nTry again with simplicial facets.  No neighboring facets listed for vertices.\n";
        os << "Vertex and vertices w/o runId:\n";
        os << v;
        q.defineVertexNeighborFacets();
        os << "This time with neighborFacets() defined for all vertices:\n";
        os << v;
        cout << os.str();
        QString s= QString::fromStdString(os.str());
        QCOMPARE(s.count(": f"), 1);

        Qhull q2(r10, "v"); // Voronoi diagram
        QhullVertex v2= q2.beginVertex();
        ostringstream os2;
        os2 << "\nTry again with Voronoi diagram of simplicial facets.  Neighboring facets automatically defined for vertices.\n";
        os2 << "Vertex and vertices w/o runId:\n";
        os2 << v2;
        cout << os2.str();
        QString s2= QString::fromStdString(os2.str());
        QCOMPARE(s2.count(": f"), 1);
    }
}//t_io

}//orgQhull

#include "moc/QhullVertex_test.moc"
