/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/qhulltest/RoadTest.cpp#1 $$Change: 1490 $
** $Date: 2012/02/19 $$Author: bbarber $
**
****************************************************************************/

//pre-compiled headers
#include <iostream>
#include <stdexcept>
#include "RoadTest.h"

using std::cout;
using std::endl;

namespace orgQhull {

#//class variable

QList<RoadTest*> RoadTest::
s_testcases;

int RoadTest::
s_test_count= 0;

int RoadTest::
s_test_fail= 0;

QStringList RoadTest::
s_failed_tests;

#//Slot

//! Executed after each test
void RoadTest::
cleanup()
{
    s_test_count++;
    if(QTest::currentTestFailed()){
        recordFailedTest();
    }
}//cleanup

#//Helper

void RoadTest::
recordFailedTest()
{
    s_test_fail++;
    QString className= metaObject()->className();
    s_failed_tests << className + "::" + QTest::currentTestFunction();
}

#//class function

int RoadTest::
runTests(QStringList arguments)
{
    int result= 0; // assume success

    foreach(RoadTest *testcase, s_testcases){
        try{
            result += QTest::qExec(testcase, arguments);
        }catch(const std::exception &e){
            cout << "FAIL!  : Threw error ";
            cout << e.what() << endl;
    s_test_count++;
            testcase->recordFailedTest();
            // Qt 4.5.2 OK.  In Qt 4.3.3, qtestcase did not clear currentTestObject
        }
    }
    if(s_test_fail){
        cout << "Failed " << s_test_fail << " of " << s_test_count << " tests.\n";
        cout << s_failed_tests.join("\n").toLocal8Bit().constData() << std::endl;
    }else{
        cout << "Passed " << s_test_count << " tests.\n";
    }
    return result;
}//runTests

}//orgQhull

#include "moc/moc_RoadTest.cpp"
