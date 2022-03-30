/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/qhulltest/QhullVertexSet_test.cpp#3 $$Change: 1464 $
** $DateTime: 2012/01/25 22:58:41 $$Author: bbarber $
**
****************************************************************************/

#include <iostream>
#include "../road/RoadTest.h" // FIXUP First for QHULL_USES_QT

#include "Qhull.h"
#include "QhullError.h"
#include "QhullFacet.h"
#include "QhullFacetSet.h"

using std::cout;
using std::endl;
using std::ostringstream;
using std::ostream;
using std::string;

namespace orgQhull {

class QhullFacetSet_test : public RoadTest
{
    Q_OBJECT

#//Test slots
private slots:
    void cleanup();
    void t_construct();
    void t_convert();
    void t_readonly();
    void t_foreach();
    void t_io();
};//QhullFacetSet_test

void
add_QhullFacetSet_test()
{
    new QhullFacetSet_test();
}

//Executed after each testcase
void QhullFacetSet_test::
cleanup()
{
    RoadTest::cleanup();
    UsingQhullLib::checkQhullMemoryEmpty();
}

void QhullFacetSet_test::
t_construct()
{
    RboxPoints rcube("c");
    Qhull q(rcube,"QR0");  // rotated unit cube
    QhullFacet f= q.firstFacet();
    QhullFacetSet fs2= f.neighborFacets();
    QVERIFY(!fs2.isEmpty());
    QCOMPARE(fs2.count(),4);
    QhullFacetSet fs4= fs2; // copy constructor
    QVERIFY(fs4==fs2);
    QhullFacetSet fs3(q.qhullQh()->facet_mergeset);
    QVERIFY(fs3.isEmpty());
}//t_construct

void QhullFacetSet_test::
t_convert()
{
    RboxPoints rcube("c");
    Qhull q(rcube,"QR0 QV2");  // rotated unit cube
    QhullFacet f= q.firstFacet();
    QhullFacetSet fs2= f.neighborFacets();
    QVERIFY(!fs2.isSelectAll());
    QCOMPARE(fs2.count(),2);
    std::vector<QhullFacet> fv= fs2.toStdVector();
    QCOMPARE(fv.size(), 2u);
    QList<QhullFacet> fv2= fs2.toQList();
    QCOMPARE(fv2.size(), 2);
    fs2.selectAll();
    QVERIFY(fs2.isSelectAll());
    std::vector<QhullFacet> fv3= fs2.toStdVector();
    QCOMPARE(fv3.size(), 4u);
    QList<QhullFacet> fv4= fs2.toQList();
    QCOMPARE(fv4.size(), 4);
}//t_convert

//! Spot check properties and read-only.  See QhullSet_test
void QhullFacetSet_test::
t_readonly()
{
    RboxPoints rcube("c");
    Qhull q(rcube,"QV0");  // good facets are adjacent to point 0
    QhullFacetSet fs= q.firstFacet().neighborFacets();
    QVERIFY(!fs.isSelectAll());
    QCOMPARE(fs.count(), 2);
    fs.selectAll();
    QVERIFY(fs.isSelectAll());
    QCOMPARE(fs.count(), 4);
    fs.selectGood();
    QVERIFY(!fs.isSelectAll());
    QCOMPARE(fs.count(), 2);
    QhullFacet f= fs.first();
    QhullFacet f2= fs.last();
    fs.selectAll();
    QVERIFY(fs.contains(f));
    QVERIFY(fs.contains(f2));
    QVERIFY(f.isGood());
    QVERIFY(!f2.isGood());
    fs.selectGood();
    QVERIFY(fs.contains(f));
    QVERIFY(!fs.contains(f2));
}//t_readonly

void QhullFacetSet_test::
t_foreach()
{
    RboxPoints rcube("c");
    // Spot check predicates and accessors.  See QhullLinkedList_test
    Qhull q(rcube,"QR0");  // rotated unit cube
    QhullFacetSet fs= q.firstFacet().neighborFacets();
    QVERIFY(!fs.contains(q.firstFacet()));
    QVERIFY(fs.contains(fs.first()));
    QhullFacet f= q.firstFacet().next();
    if(!fs.contains(f)){
        f= f.next();
    }
    QVERIFY(fs.contains(f));
    QCOMPARE(fs.first(), *fs.begin());
    QCOMPARE(*(fs.end()-1), fs.last());
}//t_foreach

void QhullFacetSet_test::
t_io()
{
    RboxPoints rcube("c");
    {
        Qhull q(rcube,"QR0 QV0");   // good facets are adjacent to point 0
        QhullFacetSet fs= q.firstFacet().neighborFacets();
        ostringstream os;
        os << fs.print(q.runId(), "Neighbors of first facet with point 0");
        os << fs.printIdentifiers("\nFacet identifiers: ");
        cout<< os.str();
        QString facets= QString::fromStdString(os.str());
        QCOMPARE(facets.count(QRegExp(" f[0-9]")), 2+13*2);
    }
}//t_io

//FIXUP -- Move conditional, QhullFacetSet code to QhullFacetSet.cpp
#ifndef QHULL_NO_STL
std::vector<QhullFacet> QhullFacetSet::
toStdVector() const
{
    QhullSetIterator<QhullFacet> i(*this);
    std::vector<QhullFacet> vs;
    while(i.hasNext()){
        QhullFacet f= i.next();
        if(isSelectAll() || f.isGood()){
            vs.push_back(f);
        }
    }
    return vs;
}//toStdVector
#endif //QHULL_NO_STL

#ifdef QHULL_USES_QT
QList<QhullFacet> QhullFacetSet::
toQList() const
{
    QhullSetIterator<QhullFacet> i(*this);
    QList<QhullFacet> vs;
    while(i.hasNext()){
        QhullFacet f= i.next();
        if(isSelectAll() || f.isGood()){
            vs.append(f);
        }
    }
    return vs;
}//toQList
#endif //QHULL_USES_QT

}//orgQhull

#include "moc/QhullFacetSet_test.moc"
