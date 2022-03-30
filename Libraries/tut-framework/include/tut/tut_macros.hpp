#ifndef TUT_MACROS_HPP
#define TUT_MACROS_HPP

#include <tut/tut.hpp>

#ifdef ensure_THROW
#error ensure_THROW macro is already defined
#endif

/** Helper macros to ensure that a call throws exception.
 * \code
 *  #include <tut_macros.h>
 *  ensure_THROW( this_function_should_throw_bad_alloc(), std::bad_alloc );
 * \endcode
 */
#define ensure_THROW(x, e) \
do                                                                         \
{                                                                          \
    try                                                                    \
    {                                                                      \
        try                                                                \
        {                                                                  \
            x;                                                             \
        }                                                                  \
        catch (const e&)                                                   \
        {                                                                  \
            break;                                                         \
        }                                                                  \
    }                                                                      \
    catch (const std::exception& ex)                                       \
    {                                                                      \
        fail(std::string(#x " has thrown unexpected exception ") +         \
             tut::type_name(ex) + ": " + ex.what());                       \
    }                                                                      \
    catch (...)                                                            \
    {                                                                      \
        fail(#x " has thrown unexpected unknown exception");               \
    }                                                                      \
    fail(#x " has not thrown expected exception " #e);                     \
} while (false)

#ifdef ensure_NO_THROW
#error ensure_NO_THROW macro is already defined
#endif

/** Helper macro to ensure a call does not throw any exceptions.
 * \code
 *  #include <tut_macros.h>
 *  ensure_NO_THROW( this_function_should_never_throw() );
 * \endcode
 */
#define ensure_NO_THROW( x ) \
try         \
{           \
    x;      \
}           \
catch(const std::exception &ex)  \
{           \
    fail( std::string(#x " has thrown unexpected exception ")+tut::type_name(ex)+": "+ex.what()); \
} \
catch(...)  \
{           \
    fail(#x " has thrown unexpected unknown exception"); \
}

#ifdef __COUNTER__
#define TUT_TESTCASE(object) template<> template<> void object::test<__COUNTER__>()
#endif

#endif

