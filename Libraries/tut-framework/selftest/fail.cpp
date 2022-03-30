#include <tut/tut.hpp>
#include <tut/tut_macros.hpp>
#include <string>
#include <stdexcept>

using std::string;
using std::runtime_error;

namespace tut
{

/**
 * Testing fail() method.
 */
struct fail_test
{
    virtual ~fail_test()
    {
    }
};

typedef test_group<fail_test> tf;
typedef tf::object object;
tf fail_test("fail");

template<>
template<>
void object::test<1>()
{
    set_test_name("checks fail with message");

    try
    {
        fail("A Fail");
        throw runtime_error("fail doesn't work");
    }
    catch (const failure& ex)
    {
        if (string(ex.what()).find("A Fail") == string::npos )
        {
            throw runtime_error("fail doesn't contain proper message");
        }
    }
}

template<>
template<>
void object::test<2>()
{
    set_test_name("checks fail without message");

    ensure_THROW( fail(), failure );
}

}

