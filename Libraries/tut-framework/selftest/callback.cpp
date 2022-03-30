
#include <tut/tut.hpp>

#include <string>
#include <stdexcept>
#include <vector>

using std::vector;
using std::string;
using std::runtime_error;

namespace tut
{
/**
 * Tests order and validity of calling callback methods
 * for various test results.
 */
struct callback_test
{
    vector<int> called;
    vector<string> grps;
    string msg;

    test_runner tr;
    struct dummy
    {
        virtual ~dummy()
        {
        }
    };

    typedef test_group<dummy> tf;
    typedef tf::object object;
    tf factory;
    tf factory2;

    enum event
    {
        RUN_STARTED = 100,
        GROUP_STARTED,
        GROUP_COMPLETED,
        TEST_COMPLETED,
        RUN_COMPLETED
    };

    struct chk_callback : public tut::callback
    {
        callback_test& ct;
        chk_callback(callback_test& c) : ct(c)
        {
        };

        void run_started()
        {
            ct.called.push_back(RUN_STARTED);
        };

        void group_started(const string& name)
        {
            ct.called.push_back(GROUP_STARTED);
            ct.grps.push_back(name);
        };

        void group_completed(const string& name)
        {
            if( ct.grps.size() == 0 )
            {
                throw runtime_error("group_completed: groups empty");
            }
            string current_group = ct.grps[ct.grps.size()-1];
            if( name != current_group )
            {
                throw runtime_error("group_completed: group mismatch: " +
                    name + " vs " + current_group);
            }
            ct.called.push_back(GROUP_COMPLETED);
            ct.grps.push_back(name + ".completed");
        };

        void test_completed(const test_result& tr)
        {
            if( ct.grps.size() == 0 )
            {
                throw runtime_error("test_completed: groups empty");
            }
            string current_group = ct.grps[ct.grps.size() - 1];
            if( tr.group != current_group )
            {
                throw runtime_error("test_completed: group mismatch: " +
                    tr.group + " vs " + current_group);
            }
            ct.called.push_back(TEST_COMPLETED);
            ct.msg = tr.message;
        };

        void run_completed()
        {
            ct.called.push_back(RUN_COMPLETED);
        };
    } callback;

    callback_test();

    virtual ~callback_test()
    {
    }
};

// ==================================
// tests of internal runner
// ==================================
template<>
template<>
void callback_test::object::test<1>()
{
    // OK
}

template<>
template<>
void callback_test::object::test<3>()
{
    throw std::runtime_error("an error");
}

callback_test::callback_test()
    : called(0),
      grps(),
      msg(),
      tr(),
      factory("internal",tr),factory2("0copy",tr), callback(*this)
{}

// ==================================
// tests of controlling runner
// ==================================
typedef test_group<callback_test> tf;
typedef tf::object object;
tf callback_test("callback");

/**
 * running one test which finished ok
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("running one test which finished ok");

    tr.set_callback(&callback);
    test_result res;
    ensure(tr.run_test("internal", 1, res));
    ensure_equals("size", called.size(), 5U);
    ensure_equals("0", called[0], RUN_STARTED);
    ensure_equals("1", called[1], GROUP_STARTED);
    ensure_equals("2", called[2], TEST_COMPLETED);
    ensure_equals("4", called[3], GROUP_COMPLETED);
    ensure_equals("5", called[4], RUN_COMPLETED);
    ensure_equals("msg", msg, "");
    ensure_equals("grp", grps[0], "internal");
    ensure_equals("grp", grps[1], "internal.completed");
}

/**
 * running one test throwing exception
 */
template<>
template<>
void object::test<2>()
{
    set_test_name("running one test throwing exception");

    tr.set_callback(&callback);
    test_result res;
    ensure(tr.run_test("internal", 3, res));
    ensure_equals("size", called.size(), 5U);
    ensure(called[0] == RUN_STARTED);
    ensure(called[1] == GROUP_STARTED);
    ensure(called[2] == TEST_COMPLETED);
    ensure(called[3] == GROUP_COMPLETED);
    ensure(called[4] == RUN_COMPLETED);
    ensure_equals("msg", msg, "an error");
    ensure_equals("grp", grps[0], "internal");
    ensure_equals("grp", grps[1], "internal.completed");
}

/**
 * running all tests in one group
 */
template<>
template<>
void object::test<3>()
{
    set_test_name("running all tests in one group");

    tr.set_callback(&callback);
    tr.run_tests("internal");
    ensure_equals("0", called[0], RUN_STARTED);
    ensure_equals("1", called[1], GROUP_STARTED);
    ensure_equals("2", called[2], TEST_COMPLETED);
    ensure_equals("3", called[3], TEST_COMPLETED);
    ensure_equals("4", called[4], GROUP_COMPLETED);
    ensure_equals("5", called[5], RUN_COMPLETED);
    ensure_equals("msg", msg, "an error");
    ensure_equals("grp[0]", grps[0], "internal");
    ensure_equals("grp[1]", grps[1], "internal.completed");
}

/**
 * running all tests in non-existing group
 */
template<>
template<>
void object::test<4>()
{
    set_test_name("running all tests in non-existing group");

    tr.set_callback(&callback);
    try
    {
        tr.run_tests("ooops!");
        fail("gotta throw an exception");
    }
    catch (const no_such_group&)
    {
    }

    ensure_equals("0", called[0], RUN_STARTED);
    ensure_equals("1", called[1], RUN_COMPLETED);
}


/**
 * running all tests in all groups
 */
template<>
template<>
void object::test<5>()
{
    set_test_name("running all tests in all groups");

    tr.set_callback(&callback);
    tr.run_tests();
    ensure_equals("0", called[0], RUN_STARTED);
    ensure_equals("1", called[1], GROUP_STARTED);
    ensure_equals("2", called[2], TEST_COMPLETED);
    ensure_equals("3", called[3], TEST_COMPLETED);
    ensure_equals("4", called[4], GROUP_COMPLETED);
    ensure_equals("5", called[5], GROUP_STARTED);
    ensure_equals("6", called[6], TEST_COMPLETED);
    ensure_equals("7", called[7], TEST_COMPLETED);
    ensure_equals("8", called[8], GROUP_COMPLETED);
    ensure_equals("9", called[9], RUN_COMPLETED);
    ensure_equals("msg", msg, "an error");
    ensure_equals("grp[0]", grps[0], "0copy");
    ensure_equals("grp[1]", grps[1], "0copy.completed");
    ensure_equals("grp[2]", grps[2], "internal");
    ensure_equals("grp[3]", grps[3], "internal.completed");
}

/**
 * running one test which doesn't exist
 */
template<>
template<>
void object::test<6>()
{
    set_test_name("running one test which doesn't exist");

    tr.set_callback(&callback);

    try
    {
        test_result res;
        ensure_not(tr.run_test("internal", 100, res));
        fail();
    }
    catch(const std::exception&)
    {
    }

    ensure_equals("size", called.size(), 4U);
    ensure_equals("0", called[0], RUN_STARTED);
    ensure_equals("1", called[1], GROUP_STARTED);
    ensure_equals("2", called[2], GROUP_COMPLETED);
    ensure_equals("3", called[3], RUN_COMPLETED);
    ensure_equals("msg", msg, "");
    ensure_equals("grp", grps[0], "internal");
    ensure_equals("grp", grps[1], "internal.completed");
}

}

