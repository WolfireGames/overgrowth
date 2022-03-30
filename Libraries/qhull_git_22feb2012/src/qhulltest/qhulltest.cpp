/****************************************************************************
**
** Copyright (c) 2008-2012 C.B. Barber. All rights reserved.
** $Id: //main/2011/qhull/src/qhulltest/qhulltest.cpp#5 $$Change: 1490 $
** $DateTime: 2012/02/19 20:27:01 $$Author: bbarber $
**
****************************************************************************/

//pre-compiled headers
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include "RoadTest.h"

#include "../libqhullcpp/RoadError.h"

using std::cout;
using std::endl;

namespace orgQhull {

void addQhullTests(QStringList &args)
{
    TESTadd_(add_QhullVertex_test); //copy

    if(args.contains("--all")){
        args.removeAll("--all");
        // up-to-date
        TESTadd_(add_Coordinates_test);
        TESTadd_(add_PointCoordinates_test);
        TESTadd_(add_QhullFacet_test);
        TESTadd_(add_QhullFacetList_test);
        TESTadd_(add_QhullFacetSet_test);
        TESTadd_(add_QhullHyperplane_test);
        TESTadd_(add_QhullLinkedList_test);
        TESTadd_(add_QhullPoint_test);
        TESTadd_(add_QhullPoints_test);
        TESTadd_(add_QhullPointSet_test);
        TESTadd_(add_QhullRidge_test);
        TESTadd_(add_QhullSet_test);
        TESTadd_(add_QhullVertex_test);
        TESTadd_(add_RboxPoints_test);
        TESTadd_(add_UsingLibQhull_test);
        // needs review
        // qhullStat
        TESTadd_(add_Qhull_test);
    }//--all
}//addQhullTests

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args= app.arguments();
    bool isAll= args.contains("--all");
    addQhullTests(args);
    int status=1010;
    try{
        status= RoadTest::runTests(args);
    }catch(const std::exception &e){
        cout << "FAIL!  : runTests() did not catch error\n";
        cout << e.what() << endl;
        if(!RoadError::emptyGlobalLog()){
            cout << RoadError::stringGlobalLog() << endl;
            RoadError::clearGlobalLog();
        }
    }
    if(!RoadError::emptyGlobalLog()){
        cout << RoadError::stringGlobalLog() << endl;
        RoadError::clearGlobalLog();
    }
    if(isAll){
        cout << "Finished test of libqhullcpp.  Test libqhull with eg/q_test" << endl;
    }else{
        cout << "Finished test of one class.  Test all classes with 'qhulltest --all'" << endl;
    }
    return status;
}

}//orgQhull

int main(int argc, char *argv[])
{
    return orgQhull::main(argc, argv); // Needs RoadTest:: for TESTadd_() linkage
}

