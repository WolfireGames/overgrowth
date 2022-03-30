#ifndef TUT_ASSERT_H_GUARD
#define TUT_ASSERT_H_GUARD
#include <tut/tut_config.hpp>

#include <limits>
#include <iomanip>
#include <iterator>
#include <cassert>
#include <cmath>

#if defined(TUT_USE_POSIX)
#include <errno.h>
#include <cstring>
#endif

#include "tut_exception.hpp"

namespace tut
{

    namespace detail
    {
        template<typename M>
        std::ostringstream &msg_prefix(std::ostringstream &str, const M &msg)
        {
            std::ostringstream ss;
            ss << msg;

            if(!ss.str().empty())
            {
                str << msg << ": ";
            }

            return str;
        }
    }


namespace
{

/**
 * Tests provided condition.
 * Throws if false.
 */
void ensure(bool cond)
{
    if (!cond)
    {
        // TODO: default ctor?
        throw failure("");
    }
}

/**
 * Tests provided condition.
 * Throws if true.
 */
void ensure_not(bool cond)
{
    ensure(!cond);
}

/**
 * Tests provided condition.
 * Throws if false.
 */
template <typename M>
void ensure(const M& msg, bool cond)
{
    if (!cond)
    {
        throw failure(msg);
    }
}

/**
 * Tests provided condition.
 * Throws if true.
 */
template <typename M>
void ensure_not(const M& msg, bool cond)
{
    ensure(msg, !cond);
}

/**
 * Tests two objects for being equal.
 * Throws if false.
 *
 * NB: both LHS and RHS must have operator << defined somewhere, or
 * client code will not compile at all!
 */
template <typename M, typename LHS, typename RHS>
void ensure_equals(const M& msg, const LHS& actual, const RHS& expected)
{
    if (expected != actual)
    {
        std::ostringstream ss;
        detail::msg_prefix(ss,msg)
           << "expected `"
           << expected
           << "` actual `"
           << actual
           << "`";
        throw failure(ss.str());
    }
}

/**
 * Tests two pointers for being equal.
 * Throws if false.
 *
 * NB: both T and Q must have operator << defined somewhere, or
 * client code will not compile at all!
 */
template <typename M, typename LHS, typename RHS>
void ensure_equals(const M& msg, const LHS * const actual, const RHS * const expected)
{
    if (expected != actual)
    {
        std::ostringstream ss;
        detail::msg_prefix(ss,msg)
           << "expected `"
           << (void*)expected
           << "` actual `"
           << (void*)actual
           << "`";
        throw failure(ss.str());
    }
}

template<typename M>
void ensure_equals(const M& msg, const double& actual, const double& expected, const double& epsilon)
{
    const double diff = actual - expected;

    if ( (actual != expected) && !((diff <= epsilon) && (diff >= -epsilon )) )
    {
        std::ostringstream ss;
        detail::msg_prefix(ss,msg)
           << std::scientific
           << std::showpoint
           << std::setprecision(16)
           << "expected `" << expected
           << "` actual `" << actual
           << "` with precision `" << epsilon << "`";
        throw failure(ss.str());
    }
}

template<typename M>
void ensure_equals(const M& msg, const double& actual, const double& expected)
{
    ensure_equals(msg, actual, expected, std::numeric_limits<double>::epsilon());
}

template <typename LHS, typename RHS>
void ensure_equals(const LHS& actual, const RHS& expected)
{
    ensure_equals("Values are not equal", actual, expected);
}


template<typename LhsIterator, typename RhsIterator>
void ensure_equals(const std::string &msg,
                   const LhsIterator &lhs_begin, const LhsIterator &lhs_end,
                   const RhsIterator &rhs_begin, const RhsIterator &rhs_end)
{
    typename std::iterator_traits<LhsIterator>::difference_type lhs_size = std::distance(lhs_begin, lhs_end);
    typename std::iterator_traits<RhsIterator>::difference_type rhs_size = std::distance(rhs_begin, rhs_end);

    if(lhs_size < rhs_size)
    {
        ensure_equals(msg + ": range is too short", lhs_size, rhs_size);
    }

    if(lhs_size > rhs_size)
    {
        ensure_equals(msg + ": range is too long", lhs_size, rhs_size);
    }

    assert(lhs_size == rhs_size);

    LhsIterator lhs_i = lhs_begin;
    RhsIterator rhs_i = rhs_begin;
    while( (lhs_i != lhs_end) && (rhs_i != rhs_end) )
    {
        if(*lhs_i != *rhs_i)
        {
            std::ostringstream ss;
            detail::msg_prefix(ss,msg)
                << "expected `" << *rhs_i
                << "` actual `" << *lhs_i
                << "` at offset " << std::distance(lhs_begin, lhs_i);
            throw failure(ss.str());
        }

        lhs_i++;
        rhs_i++;
    }

    assert(lhs_i == lhs_end);
    assert(rhs_i == rhs_end);
}

template<typename LhsIterator, typename RhsIterator>
void ensure_equals(const LhsIterator &lhs_begin, const LhsIterator &lhs_end,
                   const RhsIterator &rhs_begin, const RhsIterator &rhs_end)
{
    ensure_equals("Ranges are not equal", lhs_begin, lhs_end, rhs_begin, rhs_end);
}

template<typename LhsType, typename RhsType>
void ensure_equals(const LhsType *lhs_begin, const LhsType *lhs_end,
                   const RhsType *rhs_begin, const RhsType *rhs_end)
{
    ensure_equals("Ranges are not equal", lhs_begin, lhs_end, rhs_begin, rhs_end);
}

/**
 * Tests two objects for being at most in given distance one from another.
 * Borders are excluded.
 * Throws if false.
 *
 * NB: T must have operator << defined somewhere, or
 * client code will not compile at all! Also, T shall have
 * operators + and -, and be comparable.
 *
 * TODO: domains are wrong, T - T might not yield T, but Q
 */
template <typename M, class T>
void ensure_distance(const M& msg, const T& actual, const T& expected, const T& distance)
{
    if (expected-distance >= actual || expected+distance <= actual)
    {
        std::ostringstream ss;
        detail::msg_prefix(ss,msg)
            << " expected `"
            << expected-distance
            << "` - `"
            << expected+distance
            << "` actual `"
            << actual
            << "`";
        throw failure(ss.str());
    }
}

template <class T>
void ensure_distance(const T& actual, const T& expected, const T& distance)
{
    ensure_distance<>("Distance is wrong", actual, expected, distance);
}

template<typename M>
void ensure_errno(const M& msg, bool cond)
{
    if(!cond)
    {
#if defined(TUT_USE_POSIX)
        char e[512];
        std::ostringstream ss;
        detail::msg_prefix(ss,msg)
           << strerror_r(errno, e, sizeof(e));
        throw failure(ss.str());
#else
        throw failure(msg);
#endif
    }
}

/**
 * Unconditionally fails with message.
 */
void fail(const char* msg = "")
{
    throw failure(msg);
}

template<typename M>
void fail(const M& msg)
{
    throw failure(msg);
}

/**
 * Mark test case as known failure and skip execution.
 */
void skip(const char* msg = "")
{
    throw skipped(msg);
}

template<typename M>
void skip(const M& msg)
{
    throw skipped(msg);
}

} // end of namespace

}

#endif

