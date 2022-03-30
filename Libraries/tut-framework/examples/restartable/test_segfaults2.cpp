#include <tut/tut.hpp>

/**
 * This example test group imitates very poor code with
 * numerous segmentation faults.
 */
namespace tut
{

struct segfault_data_2
{
    virtual ~segfault_data_2() { }
};

typedef test_group<segfault_data_2> tg;
typedef tg::object object;
tg segfault_group2("seg fault 2");

template<>
template<>
void object::test<1>()
{
}

template<>
template<>
void object::test<2>()
{
    *((char*)0) = 'x';
}

template<>
template<>
void object::test<3>()
{
}

template<>
template<>
void object::test<4>()
{
    // OK
}

template<>
template<>
void object::test<5>()
{
    *((char*)0) = 'x';
}

template<>
template<>
void object::test<6>()
{
    // OK
}

}

