#ifndef TUT_EXCEPTION_H_GUARD
#define TUT_EXCEPTION_H_GUARD

#include <stdexcept>
#include "tut_result.hpp"

namespace tut
{

/**
 * The base for all TUT exceptions.
 */
struct tut_error : public std::exception
{
    explicit tut_error(const std::string& msg)
        : err_msg(msg)
    {
    }

    virtual test_result::result_type result() const
    {
        return test_result::ex;
    }

    virtual std::string type() const
    {
        return "tut::tut_error";
    }

    const char* what() const throw()
    {
        return err_msg.c_str();
    }

    ~tut_error() throw()
    {
    }

private:
    void operator=(const tut_error &);

    const std::string err_msg;
};

/**
 * Group not found exception.
 */
struct no_such_group : public tut_error
{
    explicit no_such_group(const std::string& grp)
        : tut_error(grp)
    {
    }

    virtual std::string type() const
    {
        return "tut::no_such_group";
    }

    ~no_such_group() throw()
    {
    }
};

/**
 * Test not found exception.
 */
struct no_such_test : public tut_error
{
    explicit no_such_test(const std::string& grp)
        : tut_error(grp)
    {
    }

    virtual std::string type() const
    {
        return "tut::no_such_test";
    }

    ~no_such_test() throw()
    {
    }
};

/**
 * Internal exception to be throwed when
 * test constructor has failed.
 */
struct bad_ctor : public tut_error
{
    explicit bad_ctor(const std::string& msg)
        : tut_error(msg)
    {
    }

    test_result::result_type result() const
    {
        return test_result::ex_ctor;
    }

    virtual std::string type() const
    {
        return "tut::bad_ctor";
    }

    ~bad_ctor() throw()
    {
    }
};

/**
 * Exception to be throwed when ensure() fails or fail() called.
 */
struct failure : public tut_error
{
    explicit failure(const std::string& msg)
        : tut_error(msg)
    {
    }

    test_result::result_type result() const
    {
        return test_result::fail;
    }

    virtual std::string type() const
    {
        return "tut::failure";
    }

    ~failure() throw()
    {
    }
};

/**
 * Exception to be throwed when test desctructor throwed an exception.
 */
struct warning : public tut_error
{
    explicit warning(const std::string& msg)
        : tut_error(msg)
    {
    }

    test_result::result_type result() const
    {
        return test_result::warn;
    }

    virtual std::string type() const
    {
        return "tut::warning";
    }

    ~warning() throw()
    {
    }
};

/**
 * Exception to be throwed when test issued SEH (Win32)
 */
struct seh : public tut_error
{
    explicit seh(const std::string& msg)
        : tut_error(msg)
    {
    }

    virtual test_result::result_type result() const
    {
        return test_result::term;
    }

    virtual std::string type() const
    {
        return "tut::seh";
    }

    ~seh() throw()
    {
    }
};

/**
 * Exception to be throwed when child processes fail.
 */
struct rethrown : public failure
{
    explicit rethrown(const test_result &result)
        : failure(result.message), tr(result)
    {
    }

    virtual test_result::result_type result() const
    {
        return test_result::rethrown;
    }

    virtual std::string type() const
    {
        return "tut::rethrown";
    }

    ~rethrown() throw()
    {
    }

    const test_result tr;
};

struct skipped : public tut_error
{
    explicit skipped(const std::string& msg)
        : tut_error(msg)
    {
    }

    virtual test_result::result_type result() const
    {
        return test_result::skipped;
    }

    virtual std::string type() const
    {
        return "tut::skipped";
    }

    ~skipped() throw()
    {
    }
};

}

#endif
