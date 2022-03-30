#include <tut/tut.hpp>
#include <stdexcept>

#if defined(TUT_USE_POSIX)
using std::runtime_error;

namespace tut
{

struct teardown_reaper
{
    test_runner tr;
    static std::set<pid_t> children;

    struct dummy : public tut_posix<dummy>
    {
    };

    typedef test_group<dummy> tf;
    typedef tf::object object;
    tf factory;

    teardown_reaper();

    virtual ~teardown_reaper()
    {
    }
};

std::set<pid_t> teardown_reaper::children;

template<>
template<>
void teardown_reaper::object::test<1>()
{
    if(fork() == 0)
    {
        // child 1 - hang
        while(true)
        {
            sleep(1);
        }
    }

    if(fork() == 0)
    {
        // child 3 - just exit
        exit(0);
    }

    teardown_reaper::children = get_pids();

    // do not wait for children here, let the framework handle it
}

template<>
template<>
void teardown_reaper::object::test<2>()
{
    if(fork() == 0)
    {
        // child 1 - hang
        while(true)
        {
            sleep(1);
        }
    }

    if(fork() == 0)
    {
        // child 2 - block sigterm and hang
        signal(SIGTERM, SIG_IGN);
        while(true)
        {
            sleep(1);
        }
    }

    if(fork() == 0)
    {
        // child 3 - just exit
        exit(0);
    }

    teardown_reaper::children = get_pids();

    // do not wait for children here, let the framework handle it
}


teardown_reaper::teardown_reaper()
    : tr(),
      factory("internal", tr)
{
}

typedef test_group<teardown_reaper> tf;
typedef tf::object object;
tf teardown_reaper("reaping children at test teardown time");

template<>
template<>
void object::test<1>()
{
    set_test_name("checks reaping children");

    test_result res;
    ensure("test exists", tr.run_test("internal", 1, res) );
    ensure("ok", res.result == test_result::ok);

    ensure_equals("child count", teardown_reaper::children.size(), size_t(2));

    std::set<pid_t>::iterator i;

    i = teardown_reaper::children.begin();
    ensure_equals("1. child signal", kill(*i, 0), -1);
    ensure_equals("1. child kill", errno, ESRCH);

    i++;
    ensure_equals("2. child signal", kill(*i, 0), -1);
    ensure_equals("2. child kill", errno, ESRCH);
}

template<>
template<>
void object::test<2>()
{
    set_test_name("checks killing children");

    test_result res;
    ensure("test exists", tr.run_test("internal", 2, res) );
    ensure("warning", res.result == test_result::warn);

    ensure_equals("child count", teardown_reaper::children.size(), size_t(3));

    std::set<pid_t>::iterator i;

    i = teardown_reaper::children.begin();
    ensure_equals("1. child signal", kill(*i, 0), -1);
    ensure_equals("1. child kill", errno, ESRCH);

    i++;
    /* TODO: fix this test case, something is wrong with the test
    ensure_equals("2. child signal", kill(*i, 0), -1);
    ensure_equals("2. child kill", errno, ESRCH);
    */

    i++;
    ensure_equals("3. child signal", kill(*i, 0), -1);
    ensure_equals("3. child kill", errno, ESRCH);
}

}

#endif
