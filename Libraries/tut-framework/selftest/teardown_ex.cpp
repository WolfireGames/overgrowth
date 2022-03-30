#include <tut/tut.hpp>
#include <stdexcept>

using std::runtime_error;

namespace tut
{

/**
 * Testing exceptions in teardown (cleanup) of test;
 * one run issues an integer 0 exception,
 * another -- std::exception.
 */
struct teardown_ex
{
    test_runner tr;
    struct dummy
    {
        virtual ~dummy()
        {
            static int n = 0;
#if defined(TUT_USE_SEH)
            static int d = 3;
#else
            static int d = 2;
#endif
            n++;
            if( n % d == 1 )
            {
                throw runtime_error("ex in destructor");
            }
#if defined(TUT_USE_SEH)
            else if( n % d == 0 )
            {
                // at test 3
                *((char*)0) = 0;
            }
#endif
            else
            {
                throw 0;
            }
        }
    };

    typedef test_group<dummy> tf;
    typedef tf::object object;
    tf factory;

    teardown_ex();

    virtual ~teardown_ex()
    {
    }
};

/**
 * Internal test definition
 */
template<>
template<>
void teardown_ex::object::test<1>()
{
}

template<>
template<>
void teardown_ex::object::test<2>()
{
}

template<>
template<>
void teardown_ex::object::test<3>()
{
}

template<>
template<>
void teardown_ex::object::test<4>()
{
    throw tut_error("regular");
}

teardown_ex::teardown_ex()
    : tr(),
      factory("internal", tr)
{
}

typedef test_group<teardown_ex> tf;
typedef tf::object object;
tf teardown_ex("exceptions at test teardown time");

/**
 * Checks getting std exception in.
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("checks getting std::exception");

    test_result res;
    ensure( tr.run_test("internal",1,res) );
    ensure("warning", res.result == test_result::warn);
}

/**
 * Checks getting unknown std exception.
 */
template<>
template<>
void object::test<2>()
{
    set_test_name("checks getting unknown std::exception");

    test_result res;
    ensure( tr.run_test("internal",2,res) );
    ensure("warning", res.result == test_result::warn);
}

#if defined(TUT_USE_SEH)
/**
 * Checks getting unknown std exception.
 */
template<>
template<>
void object::test<3>()
{
    set_test_name("checks getting unknown C++ exception");

    test_result res;
    ensure( tr.run_test("internal",3,res) );
    ensure_equals("warning", res.result, test_result::warn);
    ensure("warning message", res.message != "ex in destructor");
}
#endif

/**
 * Checks getting std exception in runtime.
 */
template<>
template<>
void object::test<4>()
{
    set_test_name("checks getting std::exception in runtime");

    test_result res;
    ensure( tr.run_test("internal",4,res) );
    ensure("ex", res.result == test_result::ex);
    ensure("ex message", res.message == "regular");
}

template<>
template<>
void object::test<5>()
{
    tr.run_tests("internal");
}

template<>
template<>
void object::test<6>()
{
    tr.run_tests("internal");
}

template<>
template<>
void object::test<7>()
{
    tr.run_tests();
}

template<>
template<>
void object::test<8>()
{
    tr.run_tests();
}

}

