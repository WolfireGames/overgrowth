#include <tut/tut.hpp>
#include <string>

using std::string;

namespace tut
{

struct test_callback : public callback
{
    void run_started()
    {
    }

    void test_group_started(const std::string&)
    {
    }

    void test_completed(const tut::test_result& tr)
    {
        current_test_name = tr.name;
    }

    void run_completed()
    {
    }

    test_callback()
        : current_test_name()
    {
    }

    string current_test_name;
};

struct test_name_data
{
    test_runner tr;
    test_callback callback;

    struct dummy
    {
        virtual ~dummy()
        {
        }
    };

    typedef test_group < dummy > tf;
    typedef tf::object object;
    tf factory;

    test_name_data()
        : tr(),
          callback(),
          factory("internal", tr)
    {
    }

    virtual ~test_name_data()
    {
    }
};

/**
 * Test functions under real test.
 */
template < >
template < >
void test_name_data::object::test < 1 > ()
{
    set_test_name("1");
}

template < >
template < >
void test_name_data::object::test < 2 > ()
{
    set_test_name("2");
}

template < >
template < >
void test_name_data::object::test < 3 > ()
{}

template < >
template < >
void test_name_data::object::test < 4 > ()
{
    set_test_name("failure");
    ensure(true == false);
}

template < >
template < >
void test_name_data::object::test < 5 > ()
{
    set_test_name("unexpected");
    throw "unexpected";
}

#ifdef TUT_USE_SEH
template < >
template < >
void test_name_data::object::test < 6 > ()
{
    set_test_name("seh");
    *((char*)0) = 0;
}
#endif // TUT_USE_SEH

typedef test_group < test_name_data > set_test_name_group;
typedef set_test_name_group::object set_test_name_tests;

set_test_name_group group("set_test_name");

/**
 * Tests 'set_test_name' works correctly.
 */
template < >
template < >
void set_test_name_tests::test < 1 > ()
{
    tr.set_callback(&callback);

    test_result res;

    ensure(tr.run_test("internal", 1, res));
    ensure_equals("test name", callback.current_test_name, "1");

    ensure(tr.run_test("internal", 2, res));
    ensure_equals("test name", callback.current_test_name, "2");

    ensure(tr.run_test("internal", 3, res));

    ensure_equals("test name", callback.current_test_name, "");
}

/**
 * Tests 'set_test_name' works correctly on failure.
 */
template < >
template < >
void set_test_name_tests::test < 2 > ()
{
    tr.set_callback(&callback);

    test_result res;
    ensure(tr.run_test("internal", 4, res));
    ensure_equals("test name", callback.current_test_name, "failure");
}

/**
 * Tests 'set_test_name' works correctly on unexpected exception.
 */
template < >
template < >
void set_test_name_tests::test < 3 > ()
{
    tr.set_callback(&callback);
    test_result res;
    ensure(tr.run_test("internal", 5, res));
    ensure_equals("test name", callback.current_test_name, "unexpected");
}

#ifdef TUT_USE_SEH
/**
 * Tests 'set_test_name' works correctly on structured exception.
 */
template < >
template < >
void set_test_name_tests::test < 4 > ()
{
    tr.set_callback(&callback);
    test_result res;
    ensure(tr.run_test("internal", 6, res));
    ensure_equals("test name", callback.current_test_name, "seh");
}
#endif // TUT_USE_SEH

template < >
template < >
void set_test_name_tests::test < 5 > ()
{
    ensure_equals(get_test_group(), "set_test_name");
}

}
