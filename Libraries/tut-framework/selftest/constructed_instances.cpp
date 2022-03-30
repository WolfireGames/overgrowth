#include <tut/tut.hpp>

#include <iostream>

namespace tut
{

/**
 * Testing exact number of instances created when top limit is specified.
 */
struct constructed_instances
{
    test_runner tr;
    struct dummy
    {
        static int constructed;
        dummy()
        {
            constructed++;
        }

        virtual ~dummy()
        {
        }
    };
    typedef test_group<dummy,3> tf;
    typedef tf::object object;
    tf factory;

    constructed_instances();

    virtual ~constructed_instances()
    {
    }
};

int constructed_instances::dummy::constructed = 0;

/**
 * Internal test definition
 */
template<>
template<>
void constructed_instances::object::test<1>()
{
}

template<>
template<>
void constructed_instances::object::test<3>()
{
}

/**
 * Internal constructor
 */
constructed_instances::constructed_instances()
    : tr(),
      factory("internal", tr)
{
}

typedef test_group<constructed_instances> tg;
typedef tg::object object;
tg constructed_instances("constructed instances");

/**
 * Checks two and only two instances were created.
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("checks two and only two instances were created");

    tr.run_tests("internal");
    ensure_equals("result", constructed_instances::dummy::constructed, 2);
}

}

