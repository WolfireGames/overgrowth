/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/qhulltest/QhullFacet_test.cpp#6 $$Change: 1490 $
** $DateTime: 2012/02/19 20:27:01 $$Author: bbarber $
**
****************************************************************************/

//pre-compiled headers
#include <iostream>
#include "RoadTest.h"

#include "QhullFacet.h"
#include "QhullError.h"
#include "Coordinates.h"
#include "RboxPoints.h"
#include "QhullFacetList.h"
#include "QhullFacetSet.h"
#include "QhullPointSet.h"
#include "QhullRidge.h"
#include "Qhull.h"

using std::cout;
using std::endl;
using std::ostringstream;
using std::ostream;
using std::string;

namespace orgQhull {

class QhullFacet_test : public RoadTest
{
    Q_OBJECT

#//Test slots
private slots:
    void cleanup();
    void t_constructConvert();
    void t_getSet();
    void t_value();
    void t_foreach();
    void t_io();
};//QhullFacet_test

void
add_QhullFacet_test()
{
    new QhullFacet_test();
}

//Executed after each testcase
void QhullFacet_test::
cleanup()
{
    UsingLibQhull::checkQhullMemoryEmpty();
    RoadTest::cleanup();
}

void QhullFacet_test::
t_constructConvert()
{
    // Qhull.runQhull() constructs QhullFacets as facetT
    QhullFacet f;
    QVERIFY(!f.isDefined());
    QCOMPARE(f.dimension(),0);
    RboxPoints rcube("c");
    Qhull q(rcube,"Qt QR0");  // triangulation of rotated unit cube
    QhullFacet f2(q.beginFacet());
    QCOMPARE(f2.dimension(),3);
    f= f2; // copy assignment
    QVERIFY(f.isDefined());
    QCOMPARE(f.dimension(),3);
    QhullFacet f5= f2;
    QVERIFY(f5==f2);
    QVERIFY(f5==f);
    QhullFacet f3= f2.getFacetT();
    QCOMPARE(f,f3);
    QhullFacet f4= f2.getBaseT();
    QCOMPARE(f,f4);
}//t_constructConvert

void QhullFacet_test::
t_getSet()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube,"Qt QR0");  // triangulation of rotated unit cube
        QCOMPARE(q.facetCount(), 12);
        QCOMPARE(q.vertexCount(), 8);
        QhullFacetListIterator i(q.facetList());
        while(i.hasNext()){
            const QhullFacet f= i.next();
            cout << f.id() << endl;
            QCOMPARE(f.dimension(),3);
            QVERIFY(f.id()>0 && f.id()<=39);
            QVERIFY(f.isDefined());
            if(i.hasNext()){
                QCOMPARE(f.next(), i.peekNext());
                QVERIFY(f.next()!=f);
            }
            QVERIFY(i.hasPrevious());
            QCOMPARE(f, i.peekPrevious());
        }
        QhullFacetListIterator i2(i);
        QEXPECT_FAIL("", "ListIterator copy constructor not reset to BOT", Continue);
        QVERIFY(!i2.hasPrevious());

        // test tricoplanarOwner
        QhullFacet facet = q.beginFacet();
        QhullFacet tricoplanarOwner = facet.tricoplanarOwner();
        int tricoplanarCount= 0;
        i.toFront();
        while(i.hasNext()){
            const QhullFacet f= i.next();
            if(f.tricoplanarOwner()==tricoplanarOwner){
                tricoplanarCount++;
            }
        }
        QCOMPARE(tricoplanarCount, 2);
        int tricoplanarCount2= 0;
        foreach (QhullFacet f, q.facetList()){  // Qt only
            QhullHyperplane h= f.hyperplane();
            cout << "Hyperplane: " << h << endl;
            QCOMPARE(h.count(), 3);
            QCOMPARE(h.offset(), -0.5);
            double n= h.norm();
            QCOMPARE(n, 1.0);
            QhullHyperplane hi= f.innerplane(q.runId());
            QCOMPARE(hi.count(), 3);
            double innerOffset= hi.offset()+0.5;
            cout << "InnerPlane: " << hi << "innerOffset+0.5 " << innerOffset << endl;
            QVERIFY(innerOffset >= 0.0);
            QhullHyperplane ho= f.outerplane(q.runId());
            QCOMPARE(ho.count(), 3);
            double outerOffset= ho.offset()+0.5;
            cout << "OuterPlane: " << ho << "outerOffset+0.5 " << outerOffset << endl;
            QVERIFY(outerOffset <= 0.0);
            QVERIFY(outerOffset-innerOffset < 1e-7);
            for(int k= 0; k<3; k++){
                QVERIFY(ho[k]==hi[k]);
                QVERIFY(ho[k]==h[k]);
            }
            QhullPoint center= f.getCenter(q.runId());
            cout << "Center: " << center << endl;
            double d= f.distance(center);
            QVERIFY(d < innerOffset-outerOffset);
            QhullPoint center2= f.getCenter(q.runId(), qh_PRINTcentrums);
            QCOMPARE(center, center2);
            if(f.tricoplanarOwner()==tricoplanarOwner){
                tricoplanarCount2++;
            }
        }
        QCOMPARE(tricoplanarCount2, 2);
        Qhull q2(rcube,"d Qz Qt QR0");  // 3-d triangulation of Delaunay triangulation (the cube)
        QhullFacet f2= q2.firstFacet();
        QhullPoint center3= f2.getCenter(q.runId(), qh_PRINTtriangles);
        QCOMPARE(center3.dimension(), 3);
        QhullPoint center4= f2.getCenter(q.runId());
        QCOMPARE(center4.dimension(), 3);
        for(int k= 0; k<3; k++){
            QVERIFY(center4[k]==center3[k]);
        }
        Qhull q3(rcube,"v Qz QR0");  // Voronoi diagram of a cube (one vertex)

        UsingLibQhull::setGlobalDistanceEpsilon(1e-12); // Voronoi vertices are not necessarily within distance episilon
        foreach(QhullFacet f, q3.facetList()){ //Qt only
            if(f.isGood()){
                QhullPoint p= f.voronoiVertex(q3.runId());
                cout << p.print(q3.runId(), "Voronoi vertex: ")
                    << " DistanceEpsilon " << UsingLibQhull::globalDistanceEpsilon() << endl;
                QCOMPARE(p, q3.origin());
            }
        }
    }
}//t_getSet

void QhullFacet_test::
t_value()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube, "");
        coordT c[]= {0.0, 0.0, 0.0};
        foreach (QhullFacet f, q.facetList()){  // Qt only
            double d= f.distance(q.origin());
            QCOMPARE(d, -0.5);
            double d0= f.distance(c);
            QCOMPARE(d0, -0.5);
            double facetArea= f.facetArea(q.runId());
            QCOMPARE(facetArea, 1.0);
            #if qh_MAXoutside
                double maxoutside= f.getFacetT()->maxoutside;
                QVERIFY(maxoutside<1e-7);
            #endif
        }
    }
}//t_value

void QhullFacet_test::
t_foreach()
{
    RboxPoints rcube("c W0 300");  // cube plus 300 points on its surface
    {
        Qhull q(rcube, "QR0 Qc"); // keep coplanars, thick facet, and rotate the cube
        int coplanarCount= 0;
        foreach(const QhullFacet f, q.facetList()){
            QhullPointSet coplanars= f.coplanarPoints();
            coplanarCount += coplanars.count();
            QhullFacetSet neighbors= f.neighborFacets();
            QCOMPARE(neighbors.count(), 4);
            QhullPointSet outsides= f.outsidePoints();
            QCOMPARE(outsides.count(), 0);
            QhullRidgeSet ridges= f.ridges();
            QCOMPARE(ridges.count(), 4);
            QhullVertexSet vertices= f.vertices();
            QCOMPARE(vertices.count(), 4);
            int ridgeCount= 0;
            QhullRidge r= ridges.first();
            for(int r0= r.id(); ridgeCount==0 || r.id()!=r0; r= r.nextRidge3d(f)){
                ++ridgeCount;
                if(!r.hasNextRidge3d(f)){
                    QFAIL("Unexpected simplicial facet.  They only have ridges to non-simplicial neighbors.");
                }
            }
            QCOMPARE(ridgeCount, 4);
        }
        QCOMPARE(coplanarCount, 300);
    }
}//t_foreach

void QhullFacet_test::
t_io()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube, "");
        QhullFacet f= q.beginFacet();
        cout << f;
        ostringstream os;
        os << f.printHeader(q.runId());
        os << f.printFlags("    - flags:");
        os << f.printCenter(q.runId(), qh_PRINTfacets, "    - center:");
        os << f.printRidges(q.runId());
        cout << os.str();
        ostringstream os2;
        os2 << f.print(q.runId());  // invokes print*()
        QString facetString2= QString::fromStdString(os2.str());
        facetString2.replace(QRegExp("\\s\\s+"), " ");
        ostringstream os3;
        q.setOutputStream(&os3);
        q.outputQhull("f");
        QString facetsString= QString::fromStdString(os3.str());
        QString facetString3= facetsString.mid(facetsString.indexOf("- f1\n"));
        facetString3= facetString3.left(facetString3.indexOf("\n- f")+1);
        facetString3.replace(QRegExp("\\s\\s+"), " ");
        QCOMPARE(facetString2, facetString3);
    }
}//t_io

// toQhullFacet is static_cast only

}//orgQhull

#include "moc/QhullFacet_test.moc"
