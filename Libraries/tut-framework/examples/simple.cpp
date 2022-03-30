#include <tut/tut.hpp>
#include <tut/tut_main.hpp>
#include <tut/tut_xml_reporter.hpp>

#include <exception>
#include <iostream>

namespace tut
{

    struct test
    {
        virtual ~test()
        {
        }
    };

    typedef test_group<test> tf;
    typedef tf::object object;
    tf fail_test("test()");

    template<>
    template<>
    void object::test<1>()
    {
        set_test_name("foo");
        skip();
    }


    test_runner_singleton runner;
}

int main(int argc, const char *argv[])
{
    using namespace std;
    tut::xml_reporter reporter(std::cout);
    tut::runner.get().set_callback(&reporter);

    try
    {
        if(tut::tut_main(argc, argv))
        {
            if(reporter.all_ok())
            {
                return 0;
            }
            else
            {
                std::cerr << "\nFAILURE and EXCEPTION in these tests are FAKE ;)" << std::endl;
            }
        }
    }
    catch(const tut::no_such_group &ex)
    {
        std::cerr << "No such group: " << ex.what() << std::endl;
    }
    catch(const tut::no_such_test &ex)
    {
        std::cerr << "No such test: " << ex.what() << std::endl;
    }
    catch(const tut::tut_error &ex)
    {
        std::cout << "General error: " << ex.what() << std::endl;
    }

    return 0;
}
