#include <tut/tut.hpp>
#include <string>

using std::string;

static void foo(const std::string& s)
{
    tut::ensure(s!="");
    tut::ensure_equals(s,"foo");
}

static void foo(float f)
{
    tut::ensure(f > 0.0f);
    tut::ensure_distance(f,1.0f,0.1f);
}

static void foo(int i)
{
    tut::ensure(i == 1);
    tut::ensure_equals(i,1);
}

static void foo()
{
    tut::fail();
}

namespace tut
{

static void foo(const std::string& s)
{
    ensure(s != "");
    ensure_equals(s, "foo");
}

static void foo(float f)
{
    ensure(f > 0.0f);
    ensure_distance(f, 1.0f, 0.1f);
}

static void foo(int i)
{
    ensure(i == 1);
    ensure_equals(i, 1);
}

static void foo()
{
    fail();
}

/**
 * Testing ensure*() and fail() outside test object.
 */
struct outside_test
{
    virtual ~outside_test()
    {
    }
};

typedef test_group<outside_test> tf;
typedef tf::object object;
tf ensure_outside("ensure/fail outside test object");

/**
 * Checks functions in namespace.
 */
template<>
template<>
void object::test<1>()
{
    tut::foo(string("foo"));
    tut::foo(1.05f);
    tut::foo(1);

    try
    {
        tut::foo();
    }
    catch (const failure&)
    {
    }
}

/**
 * Checks functions outside the namespace.
 */
template<>
template<>
void object::test<2>()
{
    ::foo(string("foo"));
    ::foo(1.05f);
    ::foo(1);

    try
    {
        ::foo();
    }
    catch (const failure&)
    {
    }
}

}

