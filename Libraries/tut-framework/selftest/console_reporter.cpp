#include <tut/tut.hpp>
#include <tut/tut_macros.hpp>
#include <tut/tut_console_reporter.hpp>
#include <sstream>

using std::stringstream;

namespace tut
{

/**
 * Testing reporter.
 */
struct reporter_test
{
    test_result tr1;
    test_result tr2;
    test_result tr3;
    test_result tr4;
    test_result tr5;
    test_result tr6;
    test_result tr7;
    test_result tr8;

    reporter_test()
        : tr1("foo", 1, "", test_result::ok),
          tr2("foo", 2, "", test_result::fail),
          tr3("foo", 3, "", test_result::ex),
          tr4("foo", 4, "", test_result::warn),
          tr5("foo", 5, "", test_result::term),
          tr6("foo", 6, "", test_result::skipped),
          tr7("foo", 7, "", test_result::ex_ctor),
          tr8("foo", 8, "", test_result::rethrown)
    {
    }

    virtual ~reporter_test()
    {
    }
};

typedef test_group<reporter_test> tg;
typedef tg::object object;
tg reporter_test("default reporter");

template<>
template<>
void object::test<1>()
{
    stringstream ss;
    ss << tr1 << tr2 << tr3 << tr4 << tr5 << tr6 << tr7 << tr8;
    ensure_equals("operator << formatter", ss.str(), ".[2=F][3=X][4=W][5=T][6=S][7=C][8=P]");

    ensure_THROW( ss << test_result("foo", 9, "", test_result::dummy), tut_error );
}

template<>
template<>
void object::test<2>()
{
    stringstream ss;
    console_reporter repo(ss);

    ensure_equals("ok count", repo.ok_count, 0);
    ensure_equals("fail count", repo.failures_count, 0);
    ensure_equals("ex count", repo.exceptions_count, 0);
    ensure_equals("warn count", repo.warnings_count, 0);
    ensure_equals("term count", repo.terminations_count, 0);
    ensure_equals("skip count", repo.skipped_count, 0);

    repo.run_started();
    repo.test_completed(tr1);
    repo.test_completed(tr2);
    repo.test_completed(tr2);
    repo.test_completed(tr3);
    repo.test_completed(tr3);
    repo.test_completed(tr3);
    repo.test_completed(tr4);
    repo.test_completed(tr4);
    repo.test_completed(tr4);
    repo.test_completed(tr4);
    repo.test_completed(tr5);
    repo.test_completed(tr5);
    repo.test_completed(tr5);
    repo.test_completed(tr5);
    repo.test_completed(tr5);
    repo.test_completed(tr6);
    repo.test_completed(tr6);
    repo.test_completed(tr6);
    repo.test_completed(tr6);
    repo.test_completed(tr6);
    repo.test_completed(tr6);
    repo.run_completed();

    ensure_equals("ok count", repo.ok_count, 1);
    ensure_equals("fail count", repo.failures_count, 2);
    ensure_equals("ex count", repo.exceptions_count, 3);
    ensure_equals("warn count", repo.warnings_count, 4);
    ensure_equals("term count", repo.terminations_count, 5);
    ensure_equals("skip count", repo.skipped_count, 6);
    ensure(!repo.all_ok());
}

template<>
template<>
void object::test<3>()
{
    std::stringstream ss;
    tut::console_reporter repo(ss);

    repo.run_started();
    repo.test_completed(tr1);
    repo.run_completed();

    ensure_equals("ok count",repo.ok_count,1);
    ensure(repo.all_ok());

    repo.run_started();
    ensure_equals("ok count",repo.ok_count,0);
}

template<>
template<>
void object::test<4>()
{
    stringstream ss;
    console_reporter repo(ss);

    repo.run_started();
    repo.test_completed(tr1);
    repo.run_completed();
    ensure(repo.all_ok());

    repo.run_started();
    repo.test_completed(tr1);
    repo.test_completed(tr2);
    repo.run_completed();
    ensure(!repo.all_ok());

    repo.run_started();
    repo.test_completed(tr3);
    repo.test_completed(tr1);
    repo.run_completed();
    ensure(!repo.all_ok());

    repo.run_started();
    repo.test_completed(tr1);
    repo.test_completed(tr4);
    repo.run_completed();
    ensure(!repo.all_ok());

    repo.run_started();
    repo.test_completed(tr5);
    repo.test_completed(tr1);
    repo.run_completed();
    ensure(!repo.all_ok());

    repo.run_started();
    repo.test_completed(tr1);
    repo.test_completed(tr6);
    repo.run_completed();
    ensure(repo.all_ok());
}


template<>
template<>
void object::test<5>()
{
    stringstream ss;
    console_reporter repo(ss);

    repo.run_started();
    repo.test_completed(tr1);
    repo.test_completed(tr3);
    repo.test_completed(tr6);
    repo.run_completed();

    std::string output = ss.str();
    std::string format = "\nfoo: .[3=X][6=S]\n\n---> group: foo, test: test<3>\n     problem: unexpected exception\n\n"
                         "tests summary: exceptions:1 ok:1 skipped:1\n";

    ensure_equals("output formatting mismatch", output.begin(), output.end(), format.begin(), format.end());
}

template<>
template<>
void object::test<6>()
{
    stringstream ss;
    console_reporter repo(ss);

    test_result trex("foo", 9, "", test_result::ex);
    trex.exception_typeid = "foobar";

    test_result trmsg("foo", 10, "", test_result::warn);
    trmsg.message = "barqux";

    test_result trfail("bar", 11, "", test_result::fail);
    trfail.message = "bazquz";

    repo.run_started();
    repo.test_completed(tr8);
    repo.test_completed(trex);
    repo.test_completed(trmsg);
    repo.test_completed(trfail);
    repo.run_completed();

    std::string output = ss.str();
    std::string format =
        "\n"
        "foo: [8=P][9=X][10=W]\n"
        "bar: [11=F]\n\n"
        "---> group: foo, test: test<8>\n"
        "     problem: assertion failed in child\n\n"
        "---> group: foo, test: test<9>\n"
        "     problem: unexpected exception\n"
        "     exception typeid: foobar\n\n"
        "---> group: foo, test: test<10>\n"
        "     problem: test passed, but cleanup code (destructor) raised an exception\n"
        "     message: `barqux`\n\n"
        "---> group: bar, test: test<11>\n"
        "     problem: assertion failed\n"
        "     failed assertion: `bazquz`\n\n"
        "tests summary: exceptions:1 failures:2 warnings:1 ok:0\n";

    ensure_equals("output formatting mismatch", output.begin(), output.end(), format.begin(), format.end());
}

}

