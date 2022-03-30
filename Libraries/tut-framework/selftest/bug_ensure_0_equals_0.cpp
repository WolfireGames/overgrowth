#include <tut/tut.hpp>

namespace tut
{

struct basicCompareGroup
{
    virtual ~basicCompareGroup()
    {
    }
};

typedef test_group<basicCompareGroup> typeTestgroup;
typedef typeTestgroup::object testobject;
typeTestgroup basicCompareGroup("basicCompare");

template<>
template<>
void testobject::test<1>()
{
    set_test_name("0 == 0");

    ensure("null", 0 == 0);
}

}

