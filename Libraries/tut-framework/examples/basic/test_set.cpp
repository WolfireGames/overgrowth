#include <tut/tut.hpp>
#include <set>
#include <algorithm>

using std::set;

namespace tut
{

struct set_basic
{
    set<int> s;

    set_basic(): s() { }
    virtual ~set_basic() { }
};

typedef test_group<set_basic> factory;
typedef factory::object object;
}

namespace
{

tut::factory tf("std::set basic operations");

}

namespace tut
{
/**
 * Checks insert operation
 */
template<>
template<>
void object::test<1>()
{
    s.insert(s.end(), 100);
    ensure(s.find(100) != s.end());
}

/**
 * Checks clear operation
 */
template<>
template<>
void object::test<2>()
{
    s.clear();
    ensure_equals("size is 0", s.size(), 0U);
    ensure("empty", s.empty());

    // imitate failure of container implementation
    ensure("s.end() == s.begin()", s.end() != s.begin());
}

}
