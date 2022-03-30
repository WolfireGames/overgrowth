#ifndef TUT_RESULT_H_GUARD
#define TUT_RESULT_H_GUARD
#include <tut/tut_config.hpp>

#include <string>

#if defined(TUT_USE_RTTI)
#if (defined(_MSC_VER) && !defined(_CPPRTTI)) || (defined(__GNUC__) && !defined(__GXX_RTTI))
#undef TUT_USE_RTTI
#endif
#endif

#if defined(TUT_USE_RTTI)
#include <typeinfo>
#endif

#if defined(TUT_USE_POSIX)
#include <sys/types.h>
#include <unistd.h>
#endif

namespace tut
{

#if defined(TUT_USE_RTTI)
template<typename T>
inline std::string type_name(const T& t)
{
    return typeid(t).name();
}
#else
template<typename T>
inline std::string type_name(const T& t)
{
    return "Unknown type, RTTI disabled";
}

inline std::string type_name(const std::exception&)
{
    return "Unknown std::exception, RTTI disabled";
}
#endif


#if defined(TUT_USE_POSIX)
struct test_result_posix
{
    test_result_posix()
        : pid(getpid())
    {
    }

    virtual ~test_result_posix()
    {
    }

    pid_t pid;
};
#else
struct test_result_posix
{
    virtual ~test_result_posix()
    {
    }
};
#endif

/**
 * Return type of runned test/test group.
 *
 * For test: contains result of test and, possible, message
 * for failure or exception.
 */
struct test_result : public test_result_posix
{
    /**
     * Test group name.
     */
    std::string group;

    /**
     * Test number in group.
     */
    int test;

    /**
     * Test name (optional)
     */
    std::string name;

    /**
     * result of a test
     */
    enum result_type
    {
        ok,       ///< test finished successfully
        fail,     ///< test failed with ensure() or fail() methods
        ex,       ///< test throwed an exceptions
        warn,     ///< test finished successfully, but test destructor throwed
        term,     ///< test forced test application to terminate abnormally
        ex_ctor,  ///<
        rethrown, ///<
        skipped,  ///<
        dummy     ///<
    };

    result_type result;

    /**
     * Exception message for failed test.
     */
    std::string message;
    std::string exception_typeid;

    /**
     * Default constructor.
     */
    test_result()
        : group(),
          test(0),
          name(),
          result(ok),
          message(),
          exception_typeid()
    {
    }

    /**
     * Constructor.
     */
    test_result(const std::string& grp, int pos,
                const std::string& test_name, result_type res)
        : group(grp),
          test(pos),
          name(test_name),
          result(res),
          message(),
          exception_typeid()
    {
    }

    /**
     * Constructor with exception.
     */
    test_result(const std::string& grp,int pos,
                const std::string& test_name, result_type res,
                const std::exception& ex)
        : group(grp),
          test(pos),
          name(test_name),
          result(res),
          message(ex.what()),
          exception_typeid(type_name(ex))
    {
    }

    /** Constructor with typeid.
    */
    test_result(const std::string& grp,int pos,
                const std::string& test_name, result_type res,
                const std::string& ex_typeid,
                const std::string& msg)
        : group(grp),
          test(pos),
          name(test_name),
          result(res),
          message(msg),
          exception_typeid(ex_typeid)
    {
    }

    virtual ~test_result()
    {
    }
};

}

#endif
