#include <tut/tut.hpp>
#include <tut/tut_console_reporter.hpp>
#include <tut/tut_cppunit_reporter.hpp>
#include <tut/tut_main.hpp>
#include <tut/tut_macros.hpp>
#include <iostream>

namespace tut
{
    struct main_test
    {
        main_test()
            : cb(tut::runner.get().get_callbacks())
        {
            tut::runner.get().clear_callbacks();
        }

        virtual ~main_test()
        {
            for(callbacks::iterator i = cb.begin(); i != cb.end(); ++i)
            {
                tut::runner.get().insert_callback(*i);
            }
        }

        callbacks cb;
    };

    typedef test_group<main_test> tf;
    typedef tf::object object;
    tf main_test("main");

    template<>
    template<>
    void object::test<1>()
    {
        const char *argv1[] = { "self_test", "dummy", "five" };
        ensure_THROW( tut_main(3, argv1), no_such_test );

        const char *argv2[] = { "self_test", "dummy", "5" };
        ensure_THROW( tut_main(3, argv2), no_such_group );

        const char *argv3[] = { "self_test", "main", "99" };
        ensure_THROW( tut_main(3, argv3), no_such_test );
    }

    template<>
    template<>
    void object::test<2>()
    {
        const char *argv1[] = { "self_test", "ensure", "1" };
        ensure_equals( tut_main(3, argv1), true );

        const char *argv2[] = { "self_test", "ensure" };
        ensure_equals( tut_main(2, argv2), true );
    }

    template<>
    template<>
    void object::test<3>()
    {
        std::string line;

        {
            std::stringstream ss;
            const char *argv1[] = { "self_test", "--help" };
            ensure_equals( tut_main(2, argv1, ss), false );
            std::getline(ss, line);
            ensure_equals(line, "Usage: self_test [group] [testcase]");
        }

        {
            std::stringstream ss;
            const char *argv2[] = { "self_test", "-h" };
            ensure_equals( tut_main(2, argv2, ss), false );

            const char *argv3[] = { "self_test", "/?" };
            ensure_equals( tut_main(2, argv3, ss), false );
        }
    }

    template<>
    template<>
    void object::test<4>()
    {
        std::stringstream ss;
        const char *argv2[] = { "self_test", "1", "2", "3" };
        ensure_equals( tut_main(4, argv2, ss), false );
    }

}

namespace tut
{
    test_runner_singleton runner;
}


int main(int argc, const char *argv[])
{
    tut::console_reporter reporter;
    tut::cppunit_reporter xreporter("self_test.xml");
    tut::runner.get().set_callback(&reporter);

    if(argc>1 && std::string(argv[1]) == "-x")
    {
        tut::runner.get().insert_callback(&xreporter);
        argc--;
        argv++;
    }

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
                std::cerr << std::endl;
                std::cerr << "*********************************************************" << std::endl;
                std::cerr << "WARNING: THIS VERSION OF TUT IS UNUSABLE DUE TO ERRORS!!!" << std::endl;
                std::cerr << "*********************************************************" << std::endl;
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

    return -1;
}
