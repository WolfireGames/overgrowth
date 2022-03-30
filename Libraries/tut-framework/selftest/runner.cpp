#include <tut/tut.hpp>
#include <tut/tut_macros.hpp>

namespace tut
{

struct runner_data
{
    test_runner tr;
    struct dummy
    {
        virtual ~dummy()
        {
        }
    };

    struct dummy_group : public group_base
    {
        virtual void rewind() {}
        virtual bool run_next(test_result &) { return false; }
        virtual bool run_test(int, test_result &) { return false; }
        virtual ~dummy_group()
        {
        }
    };

    typedef test_group<dummy> tf;
    typedef tf::object object;
    tf factory;

    struct dummy_callback : public tut::callback
    {
        void run_started()
        {
        }
        void test_group_started(const std::string&)
        {
        }
        void test_completed(const tut::test_result&)
        {
        }
        void run_completed()
        {
        }
    } callback;

    runner_data();

    virtual ~runner_data()
    {
    }
};

template<>
template<>
void runner_data::object::test<1>()
{
}

template<>
template<>
void runner_data::object::test<2>()
{
    throw rethrown( test_result("group", 2, "test", test_result::ok) );
}

runner_data::runner_data()
    : tr(),
      factory("runner_internal", tr),
      callback()
{
}

typedef test_group<runner_data> group;
typedef group::object object;
group testrunner("runner base functionality");

/**
 * Checks running all tests while there is no tests.
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("checks running all tests while there is no tests");

    tr.run_tests();
    tr.set_callback(&callback);
    tr.run_tests();
    tr.set_callback(0);
    tr.run_tests();
}

/**
 * Checks attempt to run test/tests in unexistent group.
 */
template<>
template<>
void object::test<2>()
{
    set_test_name("checks attempt to run test/tests in unexistent group");

    try
    {
        tr.run_tests("unexistent");
        fail("expected no_such_group");
    }
    catch (const no_such_group&)
    {
        // as expected
    }

    try
    {
        test_result r;
        tr.run_test("unexistent", 1, r);
        fail("expected tut::no_such_group");
    }
    catch (const no_such_group& )
    {
        // as expected
    }

    try
    {
        tr.set_callback(&callback);
        tr.run_tests("unexistent");
        fail("expected tut::no_such_group");
    }
    catch (const no_such_group&)
    {
        // as expected
    }

    try
    {
        tr.set_callback(&callback);
        test_result r;
        tr.run_test("unexistent", 1, r);
        fail("expected tut::no_such_group");
    }
    catch (const no_such_group&)
    {
        // as expected
    }
}

/**
 * Checks attempt to run invalid test in existent group.
 */
template<>
template<>
void object::test<3>()
{
    set_test_name("checks attempt to run invalid test in existent group");

    test_result r;
    // running non-existant test should return false
    ensure_not( tr.run_test("runner_internal", -1, r) );

    ensure_not( tr.run_test("runner_internal", 100000, r) );
}

/**
 * Checks default callback.
 */
template<>
template<>
void object::test<4>()
{
    set_test_name("checks empty callback");
    tut::callback cb;
    tr.set_callback(&cb);

    tr.run_tests();
    ensure( cb.all_ok() );
}

/**
 * Checks group operations.
 */
template<>
template<>
void object::test<5>()
{
    set_test_name("checks group operations");

    dummy_group gr;
    ensure_THROW( tr.register_group("dummy", NULL), tut_error );
    tr.register_group("dummy1", &gr);
    tr.register_group("dummy2", &gr);
    ensure_THROW( tr.register_group("dummy2", &gr), tut_error );

    tut::groupnames gn;
    gn.push_back("dummy1");
    gn.push_back("dummy2");
    gn.push_back("runner_internal");

    tut::groupnames tg = tr.list_groups();
    ensure_equals(tg.begin(), tg.end(), gn.begin(), gn.end());
}

}
