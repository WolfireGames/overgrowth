#include <tut/tut.hpp>
#include <stdexcept>

using std::runtime_error;

namespace tut
{

/**
 * Testing we can create 55-tests group.
 */
struct more_than_50
{
    test_runner tr;
    struct dummy
    {
        static bool called;

        virtual ~dummy()
        {
        }
    };
    typedef test_group<dummy,55> tf;
    typedef tf::object object;
    tf factory;

    more_than_50();

    virtual ~more_than_50()
    {
    }
};

bool more_than_50::dummy::called = false;

/**
 * Internal test definition
 */
template<>
template<>
void more_than_50::object::test<1>()
{
    if (!called)
    {
        throw runtime_error("not called 55");
    }
}

template<>
template<>
void more_than_50::object::test<55>()
{
    called = true;
}

/**
 * Internal constructor
 */
more_than_50::more_than_50()
    : tr(),
      factory("internal", tr)
{
}

typedef test_group<more_than_50> tg;
typedef tg::object object;
tg more_than_50("more than default 50 tests");

/**
 * Checks running all (and call 55th test) and then only 1th.
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("checks running all (and call 55th test) and then only 1th");

    tr.run_tests("internal");

    test_result res;
    ensure(tr.run_test("internal",1,res));
    ensure_equals("result", res.result, test_result::ok);
}

}

