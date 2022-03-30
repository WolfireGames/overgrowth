/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/qhulltest/QhullPoint_test.cpp#4 $$Change: 1490 $
** $DateTime: 2012/02/19 20:27:01 $$Author: bbarber $
**
****************************************************************************/

//pre-compiled headers
#include <iostream>
#include "RoadTest.h"

#include "QhullPoint.h"
#include "Coordinates.h"
#include "RboxPoints.h"
#include "QhullError.h"
#include "QhullFacet.h"
#include "QhullPoint.h"
#include "Qhull.h"

#include <numeric>

using std::cout;
using std::endl;
using std::ostringstream;
using std::ostream;
using std::string;

namespace orgQhull {

class QhullPoint_test : public RoadTest
{
    Q_OBJECT

#//Test slots
private slots:
    void cleanup();
    void t_construct();
    void t_convert();
    void t_readonly();
    void t_define();
    void t_operator();
    void t_iterator();
    void t_const_iterator();
    void t_qhullpoint_iterator();
    void t_io();
};//QhullPoint_test

void
add_QhullPoint_test()
{
    new QhullPoint_test();
}

//Executed after each test
void QhullPoint_test::
cleanup()
{
    UsingLibQhull::checkQhullMemoryEmpty();
    RoadTest::cleanup();
}

void QhullPoint_test::
t_construct()
{
    // Qhull.runQhull() constructs QhullFacets as facetT
    QhullPoint p;
    QVERIFY(!p.isDefined());
    QCOMPARE(p.dimension(),0);
    QCOMPARE(p.coordinates(),static_cast<double *>(0));
    RboxPoints rcube("c");
    Qhull q(rcube,"Qt QR0");  // triangulation of rotated unit cube
    QhullVertex v2(q.beginVertex());
    QhullPoint p2(v2.point());
    QVERIFY(p2.isDefined());
    QCOMPARE(p2.dimension(),3);
    // p= p2;  // copy assignment disabled, ambiguous
    QhullPoint p3(p2.dimension(), p2.coordinates());
    QCOMPARE(p2, p3);
    Coordinates c;
    c << 0.0 << 0.0 << 0.0;
    QhullPoint p6(c);
    QCOMPARE(p6, q.origin());
    QhullPoint p5= p2; // copy constructor
    QVERIFY(p5==p2);
}//t_construct

void QhullPoint_test::
t_convert()
{
    RboxPoints rcube("c");
    Qhull q(rcube,"Qt QR0");  // triangulation of rotated unit cube
    QhullVertex v= q.firstVertex();
    QhullPoint p= v.point();
    std::vector<double> vs= p.toStdVector();
    QCOMPARE(vs.size(), 3u);
    for(int k=3; k--; ){
        QCOMPARE(vs[k], p[k]);
    }
    QList<double> qs= p.toQList();
    QCOMPARE(qs.size(), 3);
    for(int k=3; k--; ){
        QCOMPARE(qs[k], p[k]);
    }
}//t_convert

void QhullPoint_test::
t_readonly()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube,"Qt QR0");  // triangulation of rotated unit cube
        QhullVertexList vs= q.vertexList();
        QhullVertexListIterator i(vs);
        while(i.hasNext()){
            QhullPoint p= i.next().point();
            int id= p.id(q.runId());
            cout << "p" << id << endl;
            QVERIFY(p.isDefined());
            QCOMPARE(p.dimension(),3);
            QCOMPARE(id, p.id());
            QVERIFY(p.id()>=0 && p.id()<9);
            const coordT *c= p.coordinates();
            coordT *c2= p.coordinates();
            QCOMPARE(c, c2);
            QCOMPARE(p.dimension(), 3);
        }
        QhullPoint p2= vs.first().point();
        QhullPoint p3= vs.last().point();
        QVERIFY(p2!=p3);
        QVERIFY(p3.coordinates()!=p2.coordinates());
    }
}//t_readonly

void QhullPoint_test::
t_define()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube,"Qt QR0");  // triangulation of rotated unit cube
        QhullVertexList vs= q.vertexList();
        QhullPoint p= vs.first().point();
        QhullPoint p2= p;
        QVERIFY(p==p2);
        QhullPoint p3= vs.last().point();
        QVERIFY(p2!=p3);
        int idx= (p3.coordinates()-p2.coordinates())/p2.dimension();
        QVERIFY(idx>-8 && idx<8);
        p2.advancePoint(idx);
        QVERIFY(p2==p3);
        p2.advancePoint(-idx);
        QVERIFY(p2==p);
        p2.advancePoint(0);
        QVERIFY(p2==p);

        QhullPoint p4= p3;
        p4.defineAs(p2);
        QVERIFY(p2==p4);
        QhullPoint p5= p3;
        p5.defineAs(p2.dimension(), p2.coordinates());
        QVERIFY(p2==p5);
        QhullPoint p6= p3;
        p6.setCoordinates(p2.coordinates());
        QCOMPARE(p2.coordinates(), p6.coordinates());
        QVERIFY(p2==p6);
        p6.setDimension(2);
        QCOMPARE(p6.dimension(), 2);
        QVERIFY(p2!=p6);
    }
}//t_define

void QhullPoint_test::
t_operator()
{
    RboxPoints rcube("c");
    Qhull q(rcube,"Qt QR0");  // triangulation of rotated unit cube
    const QhullPoint p= q.firstVertex().point();
    //operator== and operator!= tested elsewhere
    const coordT *c= p.coordinates();
    for(int k=p.dimension(); k--; ){
        QCOMPARE(c[k], p[k]);
    }
    //p[0]= 10.0; // compiler error, const
    QhullPoint p2= q.firstVertex().point();
    p2[0]= 10.0;  // Overwrites point coordinate
    QCOMPARE(p2[0], 10.0);
}//t_operator

void QhullPoint_test::
t_iterator()
{
    RboxPoints rcube("c");
    {
        QhullPoint p2;
        QCOMPARE(p2.begin(), p2.end());
        Qhull q(rcube,"QR0");  // rotated unit cube
        QhullPoint p= q.firstVertex().point();
        QhullPoint::Iterator i= p.begin();
        QhullPoint::iterator i2= p.begin();
        QVERIFY(i==i2);
        QVERIFY(i>=i2);
        QVERIFY(i<=i2);
        i= p.begin();
        QVERIFY(i==i2);
        i2= p.end();
        QVERIFY(i!=i2);
        double d3= *i;
        i2--;
        double d2= *i2;
        QCOMPARE(d3, p[0]);
        QCOMPARE(d2, p[2]);
        QhullPoint::Iterator i3(i2);
        QCOMPARE(*i2, *i3);

        (i3= i)++;
        QCOMPARE((*i3), p[1]);
        QVERIFY(i==i);
        QVERIFY(i!=i2);
        QVERIFY(i<i2);
        QVERIFY(i<=i2);
        QVERIFY(i2>i);
        QVERIFY(i2>=i);

        QhullPoint::ConstIterator i4= p.begin();
        QVERIFY(i==i4); // iterator COMP const_iterator
        QVERIFY(i<=i4);
        QVERIFY(i>=i4);
        QVERIFY(i4==i); // const_iterator COMP iterator
        QVERIFY(i4<=i);
        QVERIFY(i4>=i);
        QVERIFY(i>=i4);
        QVERIFY(i4<=i);
        QVERIFY(i2!=i4);
        QVERIFY(i2>i4);
        QVERIFY(i2>=i4);
        QVERIFY(i4!=i2);
        QVERIFY(i4<i2);
        QVERIFY(i4<=i2);
        ++i4;
        QVERIFY(i<i4);
        QVERIFY(i<=i4);
        QVERIFY(i4>i);
        QVERIFY(i4>=i);

        i= p.begin();
        i2= p.begin();
        QCOMPARE(i, i2++);
        QCOMPARE(*i2, p[1]);
        QCOMPARE(++i, i2);
        QCOMPARE(i, i2--);
        QCOMPARE(i2, p.begin());
        QCOMPARE(--i, i2);
        QCOMPARE(i2 += 3, p.end());
        QCOMPARE(i2 -= 3, p.begin());
        QCOMPARE(i2+0, p.begin());
        QCOMPARE(i2+3, p.end());
        i2 += 3;
        i= i2-0;
        QCOMPARE(i, i2);
        i= i2-3;
        QCOMPARE(i, p.begin());
        QCOMPARE(i2-i, 3);

        //p.begin end tested above

        // QhullPoint is const-only
    }
}//t_iterator

void QhullPoint_test::
t_const_iterator()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube,"QR0");  // rotated unit cube
        QhullPoint p= q.firstVertex().point();
        QhullPoint::ConstIterator i= p.begin();
        QhullPoint::const_iterator i2= p.begin();
        QVERIFY(i==i2);
        QVERIFY(i>=i2);
        QVERIFY(i<=i2);
        i= p.begin();
        QVERIFY(i==i2);
        i2= p.end();
        QVERIFY(i!=i2);
        double d3= *i;
        i2--;
        double d2= *i2;
        QCOMPARE(d3, p[0]);
        QCOMPARE(d2, p[2]);
        QhullPoint::ConstIterator i3(i2);
        QCOMPARE(*i2, *i3);

        (i3= i)++;
        QCOMPARE((*i3), p[1]);
        QVERIFY(i==i);
        QVERIFY(i!=i2);
        QVERIFY(i<i2);
        QVERIFY(i<=i2);
        QVERIFY(i2>i);
        QVERIFY(i2>=i);

        // See t_iterator for const_iterator COMP iterator

        i= p.begin();
        i2= p.constBegin();
        QCOMPARE(i, i2++);
        QCOMPARE(*i2, p[1]);
        QCOMPARE(++i, i2);
        QCOMPARE(i, i2--);
        QCOMPARE(i2, p.constBegin());
        QCOMPARE(--i, i2);
        QCOMPARE(i2+=3, p.constEnd());
        QCOMPARE(i2-=3, p.constBegin());
        QCOMPARE(i2+0, p.constBegin());
        QCOMPARE(i2+3, p.constEnd());
        i2 += 3;
        i= i2-0;
        QCOMPARE(i, i2);
        i= i2-3;
        QCOMPARE(i, p.constBegin());
        QCOMPARE(i2-i, 3);

        // QhullPoint is const-only
    }
}//t_const_iterator

void QhullPoint_test::
t_qhullpoint_iterator()
{
    QhullPoint p2;
    QhullPointIterator i= p2;
    QCOMPARE(p2.dimension(), 0);
    QVERIFY(!i.hasNext());
    QVERIFY(!i.hasPrevious());
    i.toBack();
    QVERIFY(!i.hasNext());
    QVERIFY(!i.hasPrevious());

    RboxPoints rcube("c");
    Qhull q(rcube,"QR0");  // rotated unit cube
    QhullPoint p = q.firstVertex().point();
    QhullPointIterator i2(p);
    QCOMPARE(p.dimension(), 3);
    i= p;
    QVERIFY(i2.hasNext());
    QVERIFY(!i2.hasPrevious());
    QVERIFY(i.hasNext());
    QVERIFY(!i.hasPrevious());
    i2.toBack();
    i.toFront();
    QVERIFY(!i2.hasNext());
    QVERIFY(i2.hasPrevious());
    QVERIFY(i.hasNext());
    QVERIFY(!i.hasPrevious());

    // i at front, i2 at end/back, 3 coordinates
    QCOMPARE(i.peekNext(), p[0]);
    QCOMPARE(i2.peekPrevious(), p[2]);
    QCOMPARE(i2.previous(), p[2]);
    QCOMPARE(i2.previous(), p[1]);
    QCOMPARE(i2.previous(), p[0]);
    QVERIFY(!i2.hasPrevious());
    QCOMPARE(i.peekNext(), p[0]);
    // i.peekNext()= 1.0; // compiler error, i is const
    QCOMPARE(i.next(), p[0]);
    QCOMPARE(i.peekNext(), p[1]);
    QCOMPARE(i.next(), p[1]);
    QCOMPARE(i.next(), p[2]);
    QVERIFY(!i.hasNext());
    i.toFront();
    QCOMPARE(i.next(), p[0]);
}//t_qhullpoint_iterator

void QhullPoint_test::
t_io()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube, "");
        QhullPoint p= q.beginVertex().point();
        ostringstream os;
        os << "Point w/o runId:\n";
        os << p;
        os << "Point w/ runId:\n";
        os << p.print(q.runId()) << p.print(q.runId(), " and a message ");
        os << p.printWithIdentifier(q.runId(), " Point with id and a message ");
        cout << os.str();
        QString s= QString::fromStdString(os.str());
        QCOMPARE(s.count("p"), 3);
        // QCOMPARE(s.count(QRegExp("f\\d")), 3*7 + 13*3*2);
    }
}//t_io

}//orgQhull

#include "moc/QhullPoint_test.moc"
