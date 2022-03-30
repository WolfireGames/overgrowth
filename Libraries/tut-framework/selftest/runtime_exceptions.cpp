#include <tut/tut.hpp>
#include <stdexcept>

using std::runtime_error;

namespace tut
{

/**
 * Testing exceptions in run of test;
 */
struct runtime_ex
{
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

    runtime_ex();

    virtual ~runtime_ex()
    {
    }
};

typedef test_group<runtime_ex> tf;
typedef tf::object object;
tf runtime_exceptions("exceptions at test run time");

// ==================================
// tests of internal runner
// ==================================
template<>
template<>
void runtime_ex::object::test<1>()
{
    throw 0;
}

template<>
template<>
void runtime_ex::object::test<2>()
{
    throw runtime_error("throwing std exception");
}

#if defined(TUT_USE_SEH)
template<>
template<>
void runtime_ex::object::test<3>()
{
    *((char*)0) = 0;
}
#endif

runtime_ex::runtime_ex()
    : tr(),
      factory("internal", tr)
{
}

// ==================================
// tests of controlling runner
// ==================================
/**
 * Checks getting unknown exception.
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("checks getting unknown exception");

    test_result t;
    ensure( tr.run_test("internal",1,t) );
    ensure("got exception", t.result == test_result::ex);
    ensure("got message", t.message == "");
}

/**
 * Checks getting std exception.
 */
template<>
template<>
void object::test<2>()
{
    set_test_name("checks getting std exception");

    test_result t;
    ensure( tr.run_test("internal",2,t) );
    ensure("got exception", t.result == test_result::ex);
    ensure("got message", t.message == "throwing std exception");
}

#if defined(TUT_USE_SEH)
/**
 * Checks getting segfault under Win32.
 */
template<>
template<>
void object::test<3>()
{
    set_test_name("Checks getting segfault under OS Windows");

    test_result t;
    ensure( tr.run_test("internal",3,t) );
    ensure_equals("got term", t.result, test_result::term);
}
#endif

/**
 * Running all tests.
 */
template<>
template<>
void object::test<4>()
{
    set_test_name("running all tests");

    tr.run_tests("internal");
}

}

