#include <tut/tut.hpp>
#include <string>
#include <stdexcept>

using std::string;
using std::runtime_error;

namespace tut
{

/**
 * Testing ensure_equals() method.
 */
struct ensure_eq_test
{
    virtual ~ensure_eq_test()
    {
    }
};

typedef test_group<ensure_eq_test> tf;
typedef tf::object object;
tf ensure_eq_test("ensure_equals");

/**
 * Checks positive ensure_equals with simple types
 */
template<>
template<>
void object::test<1>()
{
    volatile int n = 1; // to stop optimization
    ensure_equals("1==n", 1, n);
}

/**
 * Checks positive ensure_equals with complex non-matching types
 */
template<>
template<>
void object::test<2>()
{
    set_test_name("checks positive ensure_equals with complex non-matching"
        " types");

    ensure_equals("string(foo)==foo", string("foo"), "foo");
    ensure_equals("foo==string(foo)", "foo", string("foo"));
}

/**
 * Checks positive ensure_equals with complex matching types
 */
template<>
template<>
void object::test<3>()
{
    set_test_name("checks positive ensure_equals with complex matching types");

    ensure_equals("string==string", string("foo"), string("foo"));
}

/**
 * Checks negative ensure_equals with simple types
 */
template<>
template<>
void object::test<10>()
{
    set_test_name("checks negative ensure_equals with simple types");

    volatile int n = 1; // to stop optimization
    try
    {
        ensure_equals("2!=n", 2, n);
        throw runtime_error("ensure_equals failed");
    }
    catch (const failure& ex)
    {
        if (string(ex.what()).find("2!=n") == string::npos)
        {
            throw runtime_error("contains wrong message");
        }
    }
}

/**
 * Checks negative ensure_equals with complex non-matching types
 */
template<>
template<>
void object::test<11>()
{
    set_test_name("checks negative ensure_equals with complex non-matching"
        " types");

    try
    {
        ensure_equals("string(foo)!=boo", string("foo"), "boo");
        throw runtime_error("ensure_equals failed");
    }
    catch (const failure& ex)
    {
        if (string(ex.what()).find("string(foo)!=boo") == string::npos)
        {
            throw runtime_error("contains wrong message");
        }
    }
}

/**
 * Checks negative ensure_equals with complex matching types
 */
template<>
template<>
void object::test<12>()
{
    set_test_name("checks negative ensure_equals with complex matching types");

    try
    {
        ensure_equals("string(foo)!=string(boo)", string("foo"), string("boo"));
        throw runtime_error("ensure_equals failed");
    }
    catch (const failure& ex)
    {
        if (string(ex.what()).find("string(foo)!=string(boo)") == string::npos)
        {
            throw runtime_error("contains wrong message");
        }

        if (string(ex.what()).find("expected `boo`") == string::npos)
        {
            throw runtime_error("expected is wrong");
        }

        if (string(ex.what()).find("actual `foo`") == string::npos)
        {
            throw runtime_error("actual is wrong");
        }
    }
}

/**
 * Checks positive ensure_equals with floating point type (double)
 */
template<>
template<>
void object::test<13>()
{
    double lhs = 6.28;
    double rhs = 3.14;
    lhs /= 2;

    ensure_equals("double==double", lhs, rhs);
}

/**
 * Checks negative ensure_equals with floating point type (double)
 */
template<>
template<>
void object::test<14>()
{
    double lhs = 6.28;
    double rhs = 3.14;
    lhs /= 2;

    try
    {
        ensure_equals(lhs + 2*std::numeric_limits<double>::epsilon(), rhs);
        throw runtime_error("double!=double");
    }
    catch (const failure &ex)
    {
        ensure( string(ex.what()).find("precision") != string::npos );
    }
}

/**
 * Checks positive ensure_equals with iterator range
 */
template<>
template<>
void object::test<15>()
{
    int lhs[] = { 4, 1, 2, 3 };
    int rhs[] = { 4, 1, 2, 3 };

    ensure_equals(lhs, lhs+4, rhs, rhs+4);
}

/**
 * Checks nagative ensure_equals with iterator range
 */
template<>
template<>
void object::test<16>()
{
    int lhs[] = { 4, 1, 2, 6, 7, 5 };
    int rhs[] = { 4, 1, 3, 6, 7, 3 };

    try
    {
        ensure_equals(lhs, lhs+4, rhs, rhs+4);
        throw runtime_error("ensure_equals failed");
    }
    catch (const failure& ex)
    {
        if (string(ex.what()).find("expected `3`") == string::npos)
        {
            throw runtime_error("expected is wrong");
        }

        if (string(ex.what()).find("actual `2`") == string::npos)
        {
            throw runtime_error("actual is wrong");
        }

        if (string(ex.what()).find("at offset 2") == string::npos)
        {
            throw runtime_error("offset is wrong");
        }
    }
}

namespace
{

struct Key
{
    Key(int k): k_(k) { }
    operator int() const { return k_; }

    int k_;
};


typedef std::string Value;

}

std::ostream &operator<<(std::ostream &ss, const std::pair<Key, Value> &pair)
{
    return ss << "(" << pair.first.k_ << "," << pair.second << ")";
}

/**
 * Checks nagative ensure_equals with iterator range
 */
template<>
template<>
void object::test<17>()
{
    std::map<Key, Value> lhs;
    lhs[1] = "one";
    lhs[2] = "two";
    lhs[3] = "three";
    lhs[4] = "four";

    std::map<Key, Value> rhs;

    try
    {
        ensure_equals("size test", lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        throw runtime_error("1. ensure_equals failed");
    }
    catch (const failure& ex)
    {
        if (string(ex.what()).find("size test: range is too long: expected `0` actual `4`") == string::npos)
        {
            throw runtime_error("range too long: wrong message");
        }
    }

    rhs[1] = "one";
    rhs[2] = "two";
    rhs[3] = "three";
    rhs[4] = "FOUR";

    try
    {
        ensure_equals(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        throw runtime_error("ensure_equals failed");
    }
    catch (const failure& ex)
    {
        if (string(ex.what()).find("expected `(4,FOUR)`") == string::npos)
        {
            throw runtime_error("expected is wrong");
        }

        if (string(ex.what()).find("actual `(4,four)`") == string::npos)
        {
            throw runtime_error("actual is wrong");
        }

        if (string(ex.what()).find("at offset") == string::npos)
        {
            throw runtime_error("offset is missing");
        }
    }

    lhs.clear();
    try
    {
        ensure_equals("size test", lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        throw runtime_error("2. ensure_equals failed");
    }
    catch (const failure& ex)
    {
        if (string(ex.what()).find("size test: range is too short: expected `4` actual `0`") == string::npos)
        {
            throw runtime_error("range too short: wrong message");
        }
    }
}

/**
 * Checks positive ensure_equals with pointers
 */
template<>
template<>
void object::test<18>()
{
    int value = 42;

    int *lhs = &value;
    int *rhs = &value;

    ensure_equals("int*==int*", lhs, rhs);
}

/**
 * Checks negative ensure_equals with pointers
 */
template<>
template<>
void object::test<19>()
{
    int value1 = 42;
    int value2 = 314;

    int *lhs = &value1;
    int *rhs = &value2;

    try
    {
        ensure_equals(lhs, rhs);
        throw runtime_error("int*!=int*");
    }
    catch (const failure &ex)
    {
        std::stringstream ss;
        ss << "expected `" << (void*)rhs << "` actual `" << (void*)lhs << "`";
        ensure(string(ex.what()).find(ss.str()) != string::npos );
    }
}

/**
 * Checks negative ensure_equals with string pointers
 */
template<>
template<>
void object::test<20>()
{
    const char *lhs = "lhs";
    const char *rhs = "rhs";

    try
    {
        ensure_equals(lhs, rhs);
        throw runtime_error("char*!=char*");
    }
    catch (const failure &ex)
    {
        std::stringstream ss;
        ss << "expected `" << (void*)rhs << "` actual `" << (void*)lhs << "`";
        ensure(string(ex.what()).find(ss.str()) != string::npos );
    }
}

}

