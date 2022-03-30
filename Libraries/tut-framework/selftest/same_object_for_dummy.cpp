#include <tut/tut.hpp>

namespace tut
{

/**
 * Tests if dummy tests do not recreate object.
 */
struct same_object_for_dummy_tests
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

    same_object_for_dummy_tests();

    virtual ~same_object_for_dummy_tests()
    {
    }
};

int same_object_for_dummy_tests::counter = 0;

template<>
template<>
void same_object_for_dummy_tests::object::test<1>()
{
}

template<>
template<>
void same_object_for_dummy_tests::object::test<10>()
{
}

/**
 * Internal constructor
 */
same_object_for_dummy_tests::same_object_for_dummy_tests()
    : tr(),
      factory("internal", tr)
{
}

typedef test_group<same_object_for_dummy_tests> tg;
typedef tg::object object;
tg same_object_for_dummy_tests("new test object for each test except dummies");

/**
 * Checks getting unknown exception in setup.
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("checks getting unknown exception in setup");

    tr.run_tests("internal");
    // two for tests, and one at getting final no_more_tests exception
    ensure_equals("three objects created",counter,3);
}

}

