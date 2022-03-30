#include <iostream>
#include <exception>

#include <tut/tut_restartable.hpp>
#include <tut/tut_console_reporter.hpp>

using std::cerr;
using std::endl;
using std::exception;

using tut::console_reporter;
using tut::restartable_wrapper;

namespace tut
{

test_runner_singleton runner;

}

int main()
{
    cerr << "NB: this application will be terminated by OS four times\n"
        "before you'll get test results, be patient restarting it.\n";

    try
    {
        console_reporter visi;
        restartable_wrapper restartable;

        restartable.set_callback(&visi);
        restartable.run_tests();
    }
    catch (const exception& ex)
    {
        cerr << "tut raised ex: " << ex.what() << endl;
    }

    return 0;
}


