#include <tut/tut.hpp>
#include <tut/tut_fpt.hpp>
#include <tut/tut_macros.hpp>

#include <stdexcept>

using std::runtime_error;

namespace tut
{

/**
 * Testing ensure_close() method.
 */
struct ensure_close_test
{
    virtual ~ensure_close_test()
    {
    }
};

typedef test_group<ensure_close_test> tf;
typedef tf::object object;
tf ensure_close_test("ensure_fpt");

/**
 * Checks fpt_traits
 */
template<>
template<>
void object::test<1>()
{
    set_test_name("checks fpt_traits for integers");

    typedef tut::detail::fpt_traits<int> Traits;
    const double max = std::numeric_limits<int>::max();

    ensure_equals(Traits::zero, 0);
    ensure_equals(Traits::div(0, max), 0.0);
    ensure_equals(Traits::div(0, -max), -0.0);

    ensure_equals(Traits::div(1,0),   std::numeric_limits<double>::infinity());
    ensure_equals(Traits::div(-1,0), -std::numeric_limits<double>::infinity());

    ensure_equals(Traits::div(1,2), 0.5);
}

/**
 * Checks fpt_traits
 */
template<>
template<>
void object::test<2>()
{
    set_test_name("checks fpt_traits for doubles");

    typedef tut::detail::fpt_traits<double> Traits;
    const double min = std::numeric_limits<double>::min();
    const double max = std::numeric_limits<double>::max();

    ensure_equals(Traits::zero, 0.0);
    ensure_equals(Traits::div(0.0, max), 0.0);
    ensure_equals(Traits::div(0.0, -max), -0.0);
    ensure("0/0 is not a number", Traits::div(0.0, 0.0) != 0.0);

    ensure_equals(Traits::div(1.0,0),   std::numeric_limits<double>::infinity());
    ensure_equals(Traits::div(-1.0,0), -std::numeric_limits<double>::infinity());

    ensure_equals(Traits::div(min,2.0), min);
    ensure_equals(Traits::div(max,0.5), max);

    ensure_equals(Traits::div(1,2), 0.5);
}

/**
 * Checks positive ensure_close with simple types
 */
template<>
template<>
void object::test<3>()
{
    set_test_name("checks positive ensure_close with simple types");

    ensure_close("109 = 100 +  10%", 109, 100,  10);
    ensure_close("91  = 100 -  10%",  91, 100,  10);
    ensure_close("199 = 100 + 100%", 199, 100, 100);
    ensure_close("1   = 100 - 100%",   1, 100, 100);

    ensure_close("1009 = 1000 + 1%", 1009, 1000,   1);
    ensure_close(" 991 = 1000 - 1%",  991, 1000,   1);
    ensure_close("1049 = 1000 + 5%", 1049, 1000,   5);
    ensure_close(" 951 = 1000 - 5%",  951, 1000,   5);

    ensure_close_fraction("109 = 100 + 0.1", 109, 100, 0.1);
    ensure_close_fraction(" 91 = 100 - 0.1",  91, 100, 0.1);
    ensure_close_fraction("129 = 100 + 0.3", 129, 100, 0.3);
    ensure_close_fraction(" 71 = 100 - 0.3",  71, 100, 0.3);
    ensure_close_fraction("149 = 100 + 0.5", 149, 100, 0.5);
    ensure_close_fraction(" 51 = 100 - 0.5",  51, 100, 0.5);

    ensure_close_fraction("100 = 100", 100, 100, std::numeric_limits<double>::min());
}

/**
 * Checks positive ensure_close with doubles.
 */
template<>
template<>
void object::test<4>()
{
    set_test_name("checks positive ensure_close with doubles");

    ensure_close("1.009 = 1.0 + 1%", 1.009,  1.0,   1);
    ensure_close("0.991 = 1.0 - 1%", 0.992,  1.0,   1);
    ensure_close("1.049 = 1.0 + 5%", 1.049,  1.0,   5);
    ensure_close("0.951 = 1.0 - 5%", 0.951,  1.0,   5);

    ensure_close_fraction("1.0 = 1.0", 1.0, 1.0, std::numeric_limits<double>::min());
    ensure_close_fraction("1.0 = 1.0", 1.0, 1.0, 0);
}


/**
 * Checks negative ensure_close with simple types
 */
template<>
template<>
void object::test<5>()
{
    set_test_name("checks negative ensure_close with simple types");

    ensure_THROW( ensure_close("2!~1", 2, 1, 1), failure );
    ensure_THROW( ensure_close("0!~1", 0, 1, 1), failure );
    ensure_THROW( ensure_close_fraction("2!~1", 2, 1, 0.01), failure );
    ensure_THROW( ensure_close_fraction("2!~1", 2, 1, 0.01), failure );
}

/**
 * Checks negative ensure_close with simple types
 */
template<>
template<>
void object::test<6>()
{
    set_test_name("checks negative ensure_close with simple types");

    ensure_THROW( ensure_close(2, 1, 1), failure );
    ensure_THROW( ensure_close(0, 1, 1), failure );
}

/**
 * Checks negative ensure_close with doubles.
 */
template<>
template<>
void object::test<7>()
{
    set_test_name("checks negative ensure_close with doubles");

    ensure_THROW( ensure_close("1.0!=1.02", 1.02, 1.0, 0.01), failure );
    ensure_THROW( ensure_close("1.0!=0.98", 0.98, 1.0, 0.01), failure );
}

}

#if 0
/**
 * This code should not compile because of lacking std::numeric_limits
 * for type Float.
 */
namespace
{
    struct Float
    {
        Float(double) { }

        bool operator<(const Float &) const  { return true; }
        bool operator==(const Float &) const { return false; }
        Float operator-(const Float &) const { return *this; }
        Float operator/(const Float &) const { return *this; }
        Float operator*(const Float &) const { return *this; }
    };

    std::ostream &operator<<(std::ostream &os, const Float &) { return os; }
};

template<>
template<>
void object::test<99>()
{
    ensure_close<Float>("1.050 = 1.0  /  10%", 1.050,  1.0,  10);
}
#endif

