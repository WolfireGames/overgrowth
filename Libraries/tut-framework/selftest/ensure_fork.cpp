#include <tut/tut.hpp>
#include <stdexcept>

#if defined(TUT_USE_POSIX)
#include <unistd.h>

using std::string;
using std::runtime_error;

namespace tut
{

    /**
     * Testing ensure() method.
     */
    struct ensure_fork_test : public tut_posix<ensure_fork_test>
    {
        bool foo()
        {
            pid_t pid = fork();
            if(pid == 0)
            {
                // in child
                exit(0);
            }

            ensure_child_exit(pid, 0);

            return true;
        }
    };

    typedef test_group<ensure_fork_test> tf;
    typedef tf::object object;
    tf ensure_fork_test("ensure_fork");

    template<>
    template<>
    void object::test<1>()
    {
        set_test_name("checks fork");

        pid_t pid = fork();
        if(pid != 0)
        {
            // in parent
        }
        else
        {
            // in child
        }
    }

    template<>
    template<>
    void object::test<2>()
    {
        set_test_name("checks ensure");

        pid_t pid = fork();
        if(pid != 0)
        {
            // parent
            ensure("ENSURE", true);
        }
        else
        {
            // child
            ensure("ENSURE", true);
        }
    }

    template<>
    template<>
    void object::test<3>()
    {
        set_test_name("checks negative ensure in child");

        try
        {

            pid_t pid = fork();
            if(pid != 0)
            {
                try
                {
                    ensure_child_exit(pid, 0);
                    throw runtime_error("ensure_child_exit did not throw");
                }
                catch(const failure &ex)
                {
                    string msg = ex.what();
                    if(msg.find("CHILD") == string::npos )
                    {
                        fail("ex.what has no CHILD");
                    }
                }
            }
            else
            {
                ensure("CHILD", false);
            }
        }
        catch (const runtime_error &ex)
        {
            fail(ex.what());
        }
    }

    template<>
    template<>
    void object::test<4>()
    {
        set_test_name("checks negative ensure in multiple children");

        try
        {

            pid_t pid1 = fork();
            if(pid1 != 0)
            {
                pid_t pid2 = fork();
                if(pid2 != 0)
                {
                    try
                    {
                        ensure_child_exit(pid1, 0);
                        throw runtime_error("ensure_child_exit(1) did not throw");
                    }
                    catch(const failure &ex)
                    {
                        string msg = ex.what();
                        if(msg.find("CHILD1") == string::npos )
                        {
                            fail("ex.what has no CHILD1");
                        }
                    }

                    try {
                        ensure_child_exit(pid2, 0);
                        throw runtime_error("ensure_child_exit(2) did not throw");
                    }
                    catch(const failure &ex)
                    {
                        string msg = ex.what();
                        if(msg.find("CHILD2") == string::npos )
                        {
                            fail("ex.what has no CHILD2");
                        }
                    }
                }
                else
                {
                    ensure("CHILD2", false);
                }
            }
            else
            {
                ensure("CHILD1", false);
            }
        }
        catch(const runtime_error &ex)
        {
            fail(ex.what());
        }
    }

    template<>
    template<>
    void object::test<5>()
    {
        set_test_name("checks child signal ensure");

        pid_t pid = fork();
        if(pid != 0)
        {
            ensure_child_signal(pid, SIGSEGV);
        }
        else
        {
            raise(SIGSEGV);
        }
    }

    template<>
    template<>
    void object::test<6>()
    {
        set_test_name("checks negative child signal ensure");

        try
        {
            pid_t pid = fork();
            if(pid != 0)
            {
                try
                {
                    ensure_child_signal(pid, SIGSEGV);
                    throw runtime_error("ensure_child_signal did not throw");
                }
                catch(const failure &ex)
                {
                    string msg = ex.what();
                    if(msg.find("code 42") == string::npos ||
                       msg.find("signal 11") == string::npos)
                    {
                        fail("ex.what has no expected/actual child info");
                    }
                }
            }
            else
            {
                exit(42);
            }
        }
        catch(const std::runtime_error &ex)
        {
            fail(ex.what());
        }
    }

    template<>
    template<>
    void object::test<7>()
    {
        set_test_name("checks child exit ensure");

        pid_t pid = fork();
        if(pid != 0)
        {
            ensure_child_exit(pid, 42);
        }
        else
        {
            exit(42);
        }
    }

    template<>
    template<>
    void object::test<8>()
    {
        set_test_name("checks negative child exit ensure");

        try
        {
            pid_t pid = fork();
            if(pid != 0)
            {
                try
                {
                    ensure_child_exit(pid, 42);
                    throw runtime_error("ensure_child_exit did not throw");
                }
                catch(const failure &ex)
                {
                    string msg = ex.what();
                    if(msg.find("code 42") == string::npos ||
                       msg.find("signal 11") == string::npos)
                    {
                        fail("ex.what has no expected/actual child info");
                    }
                }
            }
            else
            {
                raise(SIGSEGV);
            }
        }
        catch(const std::runtime_error &ex)
        {
            fail(ex.what());
        }
    }

    template<>
    template<>
    void object::test<9>()
    {
        set_test_name("checks method access in test object");

        ensure( foo() );
    }

}

#endif

namespace tut
{
    struct ensure_fork_test2
    {
        virtual ~ensure_fork_test2()
        {
        }
    };

    typedef test_group<ensure_fork_test2> tf2;
    typedef tf2::object object2;
    tf2 ensure_fork_test2("ensure_fork2");

    template<>
    template<>
    void object2::test<1>()
    {
        try
        {
            throw rethrown( test_result("group", 1, "test", test_result::ex_ctor) );
        }
        catch(const rethrown &ex)
        {
            ensure_equals(ex.result(), test_result::rethrown);
            ensure_equals(ex.tr.result, test_result::ex_ctor);
        }
    }
}
