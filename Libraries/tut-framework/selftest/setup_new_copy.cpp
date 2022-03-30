#include <tut/tut.hpp>

namespace tut
{

/**
 * Testing each test starts with brand new test object.
 */
struct setup_new_copy
{
    static int counter;
    test_runner tr;
    struct dummy
    {
        dummy()
        {
            counter++;
        }

        virtual ~dummy()
        {
        }
    };

    typedef test_group<dummy> tf;
    typedef tf::object object;
    tf factory;

    setup_new_copy();

    virtual ~setup_new_copy()
    {
    }
};

int setup_new_copy::counter = 0;

/**
 * Internal test definition
 */
template<>
template<>
void setup_new_copy::object::test<1>()
{
}

/**
 * Internal constructor
 */
setup_new_copy::setup_new_copy()
    : tr(),
      factory("internal", tr)
{
}

typedef test_group<setup_new_copy> tg;
typedef tg::object object;
tg setup_new_copy("new test object for each test");

/**
 * Checks getting unknown exception in setup.
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("checks getting unknown exception in setup");

    test_result r;
    ensure(tr.run_test("internal",1,r));
    ensure_equals("one constructor called", counter, 1);

    ensure(tr.run_test("internal",1,r));
    ensure_equals("another constructor called", counter, 2);
}

}

