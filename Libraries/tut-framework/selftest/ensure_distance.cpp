#include <tut/tut.hpp>
#include <tut/tut_macros.hpp>

namespace tut
{

/**
 * Testing ensure_distance() method.
 */
struct ensure_distance_test
{
    virtual ~ensure_distance_test()
    {
    }
};

typedef test_group<ensure_distance_test> tf;
typedef tf::object object;
tf ensure_distance_test("ensure_distance");

/**
 * Checks positive ensure_distance with simple types
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("checks positive ensure_distance with simple types");

    ensure_distance("2~=1", 1, 2, 2);
    ensure_distance("0~=1", 1, 0, 2);
    ensure_distance(1, 2, 2);
    ensure_distance(1, 0, 2);
}

/**
 * Checks positive ensure_distance with doubles.
 */
template<>
template<>
void object::test<2>()
{
    set_test_name("checks positive ensure_distance with doubles");

    ensure_distance("1.0~=1.01", 1.0, 1.01, 0.011);
    ensure_distance("1.0~=0.99", 1.0, 0.99, 0.011);
    ensure_distance(1.0, 1.01, 0.011);
    ensure_distance(1.0, 0.99, 0.011);
}

/**
 * Checks negative ensure_distance with simple types
 */
template<>
template<>
void object::test<10>()
{
    set_test_name("checks negative ensure_distance with simple types");

    ensure_THROW( ensure_distance("2!~1", 2, 1, 1), failure );
    ensure_THROW( ensure_distance("0!~1", 0, 1, 1), failure );
}

/**
 * Checks negative ensure_distance with simple types
 */
template<>
template<>
void object::test<11>()
{
    set_test_name("checks negative ensure_distance with simple types");

    ensure_THROW( ensure_distance(2, 1, 1), failure );
    ensure_THROW( ensure_distance(0, 1, 1), failure );
}

/**
 * Checks negative ensure_distance with doubles.
 */
template<>
template<>
void object::test<12>()
{
    set_test_name("checks negative ensure_distance with doubles");

    ensure_THROW( ensure_distance("1.0!=1.02", 1.02, 1.0, 0.01), failure );
    ensure_THROW( ensure_distance("1.0!=0.98", 0.98, 1.0, 0.01), failure );
}

}

