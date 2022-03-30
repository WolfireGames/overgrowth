#include <tut/tut.hpp>

/**
 * This example test group imitates very poor code with
 * numerous segmentation faults.
 */
namespace tut
{

struct segfault_data
{
    virtual ~segfault_data() { }
};

typedef test_group<segfault_data> tg;
typedef tg::object object;
tg segfault_group("seg fault 1");

template<>
template<>
void object::test<1>()
{
}

template<>
template<>
void object::test<2>()
{
}

template<>
template<>
void object::test<3>()
{
    *((char*)0) = 'x';
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

