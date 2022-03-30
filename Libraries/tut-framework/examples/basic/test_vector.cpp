#include <tut/tut.hpp>
#include <vector>
#include <stdexcept>

using std::vector;
using std::logic_error;

namespace tut
{

// Data used by each test
struct vector_basic
{
    vector<int> v;

    vector_basic(): v() { }
    virtual ~vector_basic() { }
};

// Test group registration
typedef test_group<vector_basic> factory;
typedef factory::object object;

}

namespace
{

tut::factory tf("std::vector basic operations");

}

namespace tut
{
/**
 * Checks push_back operation
 */
template<>
template<>
void object::test<1>()
{
    v.push_back(100);
    ensure(v.size() == 1);
    ensure("size=1", v.size() == 1);
    ensure("v[0]=100", v[0] == 100);
}

/**
 * Checks clear operation
 */
template<>
template<>
void object::test<23>()
{
    v.clear();
    // imitation of user code exception
    throw std::logic_error("no rights");
}

/**
 * Checks resize operation
 */
template<>
template<>
void object::test<2>()
{
    v.resize(22);
    ensure_equals("capacity", 22U, v.size());
}

/**
 * Checks range constructor
 */
template<>
template<>
void object::test<3>()
{
    int c[] = { 1, 2, 3, 4 };
    v = std::vector<int>(&c[0], &c[4]);
    ensure_equals("size", v.size(), 4U);
    ensure("v[0]", v[0] == 1);
    ensure("v[1]", v[1] == 2);
    ensure("v[2]", v[2] == 3);
    ensure("v[3]", v[3] == 4);
}

}

