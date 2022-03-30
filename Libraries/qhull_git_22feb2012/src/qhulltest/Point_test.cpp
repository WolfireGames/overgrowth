/****************************************************************************
**
** Copyright (C) 2009-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/qhulltest/Point_test.cpp#4 $$Change: 1490 $
** $DateTime: 2012/02/19 20:27:01 $$Author: bbarber $
**
****************************************************************************/

#include <iostream>
using std::cout;
using std::endl;
#include "RoadTest.h" // QT_VERSION

#include "QhullPoint.h"

#include "Qhull.h"

namespace orgQhull {

class Point_test : public RoadTest
{
    Q_OBJECT

#//Test slots
private slots:
    void initTestCase();
    void t_construct();
    void t_getset();
    void t_operator();
    void t_const_iterator();
    void t_iterator();
    void t_point_iterator();
    // void t_mutable_point_iterator();
    // void t_io();
};//Point_test

void
add_Point_test()
{
    new Point_test();
}

void Point_test::
initTestCase(){
    RboxPoints rcube("c");
    Qhull q(rcube, "");
    UsingQhullLib::setGlobals();
}//initTestCase

void Point_test::
t_construct()
{
    QhullPoint p;
    QCOMPARE(p.dimension(), 0);
    coordT c[]= {0.0, 1.0, 2.0};
    QhullPoint p2;
    p2.defineAs(3, c);
    QCOMPARE(p2.dimension(), 3);
    QCOMPARE(p2.coordinates(), c);
    coordT c2[]= {0.0, 1.0, 2.0};
    QhullPoint p3(3, c2);
    QVERIFY(p3==p2);
    QhullPoint p5(p3);
    QVERIFY(p5==p3);
}//t_construct

void Point_test::
t_getset()
{
    coordT c[]= {0.0, 1.0, 2.0};
    QhullPoint p(3, c);
    QCOMPARE(p.coordinates(), c);
    QCOMPARE(p.dimension(), 3);
    QCOMPARE(p[2], 2.0);
    QhullPoint p2(p);
    p2.defineAs(p);
    QVERIFY(p2==p);
    QVERIFY(p2.coordinates()==p.coordinates());
    QVERIFY(p2.dimension()==p.dimension());
    p2.setDimension(2);
    QCOMPARE(p2.dimension(), 2);
    QVERIFY(p2!=p);
    coordT c2[]= {0.0, 1.0};
    p2.setCoordinates(c2);
    QCOMPARE(p2.coordinates(), c2);
    p.defineAs(2, c);
    QVERIFY(p2==p);
    QCOMPARE(p[1], 1.0);
}//t_getset

void Point_test::
t_operator()
{
    QhullPoint p;
    QhullPoint p2(p);
    QVERIFY(p==p2);
    QVERIFY(!(p!=p2));
    coordT c[]= {0.0, 1.0, 2.0};
    QhullPoint p3(3, c);
    QVERIFY(p3!=p2);
    QhullPoint p4(p3);
    QVERIFY(p4==p3);
    coordT c5[]= {5.0, 6.0, 7.0};
    QhullPoint p5(3, c5);
    QVERIFY(p5!=p3);
    QCOMPARE(p5[1], 6.0);
    QCOMPARE(p5[0], 5.0);
    const coordT *c0= &p5[0];
    QCOMPARE(*c0, 5.0);
}//t_operator

void Point_test::
t_const_iterator()
{
    coordT c[]= {1.0, 2.0, 3.0};
    QhullPoint p(3, c);
    QhullPoint::const_iterator i(p.coordinates());
    QhullPoint::const_iterator i2= p.coordinates();
    QVERIFY(i==i2);
    QVERIFY(i>=i2);
    QVERIFY(i<=i2);
    QCOMPARE(*i, 1.0);
    QCOMPARE(*(i+1), 2.0);
    QCOMPARE(*(i+1), i[1]);
    i= p.end();
    QVERIFY(i!=i2);
    i= i2;
    QVERIFY(i==i2);
    i= p.end();
    i= p.begin();
    QCOMPARE(*i, 1.0);
    QhullPoint::ConstIterator i3= p.end();
    QCOMPARE(*(i3-1), 3.0);
    QCOMPARE(*(i3-1), i3[-1]);
    QVERIFY(i!=i3);
    QVERIFY(i<i3);
    QVERIFY(i<=i3);
    QVERIFY(i3>i);
    QVERIFY(i3>=i);
}//t_const_iterator


void Point_test::
t_iterator()
{
    coordT c[]= {1.0, 2.0, 3.0};
    QhullPoint p(3, c);
    QhullPoint::Iterator i(p.coordinates());
    QhullPoint::iterator i2= p.coordinates();
    QVERIFY(i==i2);
    QVERIFY(i>=i2);
    QVERIFY(i<=i2);
    QCOMPARE(*i, 1.0);
    QCOMPARE(*(i+1), 2.0);
    QCOMPARE(*(i+1), i[1]);
    i= p.end();
    QVERIFY(i!=i2);
    i= i2;
    QVERIFY(i==i2);
    i= p.end();
    i= p.begin();
    QCOMPARE(*i, 1.0);
    QhullPoint::Iterator i3= p.end();
    QCOMPARE(*(i3-1), 3.0);
    QCOMPARE(*(i3-1), i3[-1]);
    QVERIFY(i!=i3);
    QVERIFY(i<i3);
    QVERIFY(i<=i3);
    QVERIFY(i3>i);
    QVERIFY(i3>=i);
    // compiler errors -- QhullPoint is const-only
    // QCOMPARE((i[0]= -10.0), -10.0);
    // coordT *c3= &i3[1];
}//t_iterator

void Point_test::
t_point_iterator()
{
    coordT c[]= {1.0, 3.0, 4.0};
    QhullPoint p(3, c);
    QhullPointIterator i(p);
    QhullPointIterator i2= p;
    QVERIFY(i2.hasNext());
    QVERIFY(!i2.hasPrevious());
    QVERIFY(i.hasNext());
    QVERIFY(!i.hasPrevious());
    i.toBack();
    i2.toFront();
    QVERIFY(!i.hasNext());
    QVERIFY(i.hasPrevious());
    QVERIFY(i2.hasNext());
    QVERIFY(!i2.hasPrevious());

    coordT c2[]= {1.0, 3.0, 4.0};
    QhullPoint p2(0, c2); // 0-dimensional
    i2= p2;
    QVERIFY(!i2.hasNext());
    QVERIFY(!i2.hasPrevious());
    i2.toBack();
    QVERIFY(!i2.hasNext());
    QVERIFY(!i2.hasPrevious());
    QCOMPARE(i.peekPrevious(), 4.0);
    QCOMPARE(i.previous(), 4.0);
    QCOMPARE(i.previous(), 3.0);
    QCOMPARE(i.previous(), 1.0);
    QVERIFY(!i.hasPrevious());
    QCOMPARE(i.peekNext(), 1.0);
    // i.peekNext()= 1.0; // compiler error
    QCOMPARE(i.next(), 1.0);
    QCOMPARE(i.peekNext(), 3.0);
    QCOMPARE(i.next(), 3.0);
    QCOMPARE(i.next(), 4.0);
    QVERIFY(!i.hasNext());
    i.toFront();
    QCOMPARE(i.next(), 1.0);
}//t_point_iterator

// No MutableQhullPointIterator since QhullPoint is const-only

#if 0

void Point_test::
t_io()
{
    QhullPoint p;
    cout<< "INFO:     empty point" << p << endl;
    const coordT c[]= {1.0, 3.0, 4.0};
    QhullPoint p2(2, c); // 2-dimensional
    cout<< "INFO:   " << p2 << endl;
}//t_io

error LNK2019: unresolved external symbol "class std::basic_ostream<char,struct std::char_traits<char> > & __cdecl operator<<(class std::basic_ostream<char,struct std::char_traits<char> > &,class orgQhull::QhullPoint &)" (??6@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@std@@AAV01@AAVPoint@orgQhull@@@Z) referenced in function "private: void __thiscall orgQhull::Point_test::t_io(void)" (?t_io@Point_test@orgQhull@@AAEXXZ)

#endif

}//orgQhull

#include "moc/Point_test.moc"
