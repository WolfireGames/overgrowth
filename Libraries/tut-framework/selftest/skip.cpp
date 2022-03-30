#include <tut/tut.hpp>
#include <tut/tut_macros.hpp>
#include <string>
#include <stdexcept>

namespace tut
{

/**
 * Testing skip() method.
 */
struct skip_test
{
    virtual ~skip_test()
    {
    }
};

typedef test_group<skip_test> tf;
typedef tf::object object;
tf skip_test("skip");

template<>
template<>
void object::test<1>()
{
    set_test_name("checks skip with message");

    try
    {
        skip("A Fail");
        throw std::runtime_error("skip doesn't work");
    }
    catch (const skipped& ex)
    {
        if (std::string(ex.what()).find("A Fail") == std::string::npos )
        {
            throw std::runtime_error("skip doesn't contain proper message");
        }
    }
}

template<>
template<>
void object::test<2>()
{
    set_test_name("checks skip without message");

    ensure_THROW( skip(), skipped );
}

}

