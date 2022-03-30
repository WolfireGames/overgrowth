#include <tut/tut.hpp>

#include <stdexcept>

using std::runtime_error;

namespace tut
{

/**
 * Testing we can create non-50-tests group.
 */
struct less_than_50
{
    test_runner tr;
    struct dummy
    {
        static bool called;

        virtual ~dummy()
        {
        }
    };
    typedef test_group<dummy,2> tf;
    typedef tf::object object;
    tf factory;

    less_than_50();

    virtual ~less_than_50()
    {
    }
};

bool less_than_50::dummy::called = false;

/**
 * Internal test definition
 */
template<>
template<>
void less_than_50::object::test<1>()
{
    if (called)
    {
        throw std::runtime_error("called 3");
    }
}

template<>
template<>
void less_than_50::object::test<3>()
{
    called = true;
}

/**
 * Internal constructor
 */
less_than_50::less_than_50()
    : tr(),
      factory("internal", tr)
{
}

typedef test_group<less_than_50> tg;
typedef tg::object object;
tg less_than_50("less than default 50 tests");

/**
 * Checks running all (and do not call 3rd test) and then only 1th.
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("checks running all (and do not call 3rd test) and then"
        " only 1th.");

    test_result res;
    ensure(tr.run_test("internal",1,res));
    ensure_equals("result", res.result, test_result::ok);
}

}

