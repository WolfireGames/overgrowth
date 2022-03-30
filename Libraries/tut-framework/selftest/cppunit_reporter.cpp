#include <tut/tut.hpp>
#include <tut/tut_macros.hpp>
#include <tut/tut_cppunit_reporter.hpp>
#include <sstream>
#include <iterator>
#include <cstdio>

using std::stringstream;

namespace tut
{

/**
 * Testing reporter.
 */
struct cppunit_reporter_test
{
    test_result tr1;
    test_result tr2;
    test_result tr3;
    test_result tr4;
    test_result tr5;
    test_result tr6;
    test_result tr7;
    test_result tr8;

    const std::string filename;

    cppunit_reporter_test()
        : tr1("ok",       1, "tr1", test_result::ok),
          tr2("fail",     2, "tr2", test_result::fail,    "", "fail message"),
          tr3("ex",       3, "tr3", test_result::ex,      "exception", "ex message"),
          tr4("warn",     4, "tr4", test_result::warn,    "", "warn message"),
          tr5("term",     5, "tr5", test_result::term,    "", "term message"),
          tr6("skipped",  6, "tr6", test_result::skipped, "", "skipped message"),
          tr7("ctor",     7, "tr7", test_result::ex_ctor, "exception", "ex_ctor message"),
          tr8("rethrown", 8, "tr8", test_result::rethrown, "exception", "rethrown message"),
          filename("cppunit_reporter.log")
    {
    }

    virtual ~cppunit_reporter_test()
    {
        remove(filename.c_str());
    }
};

typedef test_group<cppunit_reporter_test> tg;
typedef tg::object object;
tg cppunit_reporter_test("cppunit reporter");

template<>
template<>
void object::test<1>()
{
    set_test_name("tests empty run report to the stream");

    std::stringstream ss;
    cppunit_reporter repo(ss);

    repo.run_started();
    repo.run_completed();

    std::string expected =
        "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>\n"
        "<TestRun>\n"
        "  <Statistics>\n"
        "    <Tests>0</Tests>\n"
        "    <FailuresTotal>0</FailuresTotal>\n"
        "    <Errors>0</Errors>\n"
        "    <Failures>0</Failures>\n"
        "  </Statistics>\n"
        "</TestRun>\n";

    std::string actual = ss.str();

    ensure(repo.all_ok());
    ensure_equals( actual.begin(), actual.end(), expected.begin(), expected.end() );
}

template<>
template<>
void object::test<2>()
{
    set_test_name("tests empty run report to a file");

    {
        std::ifstream t(filename.c_str());
        ensure_equals( "File "+filename+" exists, remove it before running the test", t.good(), false);
    }
    {
        cppunit_reporter repo(filename.c_str());

        repo.run_started();
        repo.run_completed();

        std::string expected =
            "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>\n"
            "<TestRun>\n"
            "  <Statistics>\n"
            "    <Tests>0</Tests>\n"
            "    <FailuresTotal>0</FailuresTotal>\n"
            "    <Errors>0</Errors>\n"
            "    <Failures>0</Failures>\n"
            "  </Statistics>\n"
            "</TestRun>\n";

        std::ifstream file(filename.c_str());
        std::string actual;
        std::copy( std::istreambuf_iterator<char>(file.rdbuf()), std::istreambuf_iterator<char>(), std::back_inserter(actual) );

        ensure("repo.all_ok()", repo.all_ok());
        ensure_equals("is same", actual.begin(), actual.end(), expected.begin(), expected.end() );
    }
}

template<>
template<>
void object::test<3>()
{
    std::stringstream ss;
    cppunit_reporter repo(ss);

    repo.run_started();
    repo.test_completed(tr1);
    repo.test_completed(tr2);
    repo.test_completed(tr3);
    repo.test_completed(tr4);
    repo.test_completed(tr5);
    repo.test_completed(tr6);
    repo.test_completed(tr7);
    repo.test_completed(tr8);
    repo.run_completed();

    std::string expected =
        "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>\n"
        "<TestRun>\n"
        "  <FailedTests>\n";

    expected +=
        "    <FailedTest id=\"2\">\n"
        "      <Name>fail::tr2</Name>\n"
        "      <FailureType>Assertion</FailureType>\n"
        "      <Location>\n"
        "        <File>Unknown</File>\n"
        "        <Line>Unknown</Line>\n"
        "      </Location>\n"
        "      <Message>fail message</Message>\n"
        "    </FailedTest>\n";

    expected +=
        "    <FailedTest id=\"3\">\n"
        "      <Name>ex::tr3</Name>\n"
        "      <FailureType>Assertion</FailureType>\n"
        "      <Location>\n"
        "        <File>Unknown</File>\n"
        "        <Line>Unknown</Line>\n"
        "      </Location>\n"
        "      <Message>Thrown exception: exception\nex message</Message>\n"
        "    </FailedTest>\n";

    expected +=
        "    <FailedTest id=\"4\">\n"
        "      <Name>warn::tr4</Name>\n"
        "      <FailureType>Assertion</FailureType>\n"
        "      <Location>\n"
        "        <File>Unknown</File>\n"
        "        <Line>Unknown</Line>\n"
        "      </Location>\n"
        "      <Message>Destructor failed\nwarn message</Message>\n"
        "    </FailedTest>\n";

    expected +=
        "    <FailedTest id=\"5\">\n"
        "      <Name>term::tr5</Name>\n"
        "      <FailureType>Error</FailureType>\n"
        "      <Location>\n"
        "        <File>Unknown</File>\n"
        "        <Line>Unknown</Line>\n"
        "      </Location>\n"
        "      <Message>Test application terminated abnormally\nterm message</Message>\n"
        "    </FailedTest>\n";

    expected +=
        "    <FailedTest id=\"7\">\n"
        "      <Name>ctor::tr7</Name>\n"
        "      <FailureType>Error</FailureType>\n"
        "      <Location>\n"
        "        <File>Unknown</File>\n"
        "        <Line>Unknown</Line>\n"
        "      </Location>\n"
        "      <Message>Constructor has thrown an exception: exception\nex_ctor message</Message>\n"
        "    </FailedTest>\n";

    expected +=
        "    <FailedTest id=\"8\">\n"
        "      <Name>rethrown::tr8</Name>\n"
        "      <FailureType>Assertion</FailureType>\n"
        "      <Location>\n"
        "        <File>Unknown</File>\n"
        "        <Line>Unknown</Line>\n"
        "      </Location>\n"
        "      <Message>Child failed\nrethrown message</Message>\n"
        "    </FailedTest>\n";

    expected +=
        "  </FailedTests>\n"
        "  <SuccessfulTests>\n";

    expected +=
        "    <Test id=\"1\">\n"
        "      <Name>ok::tr1</Name>\n"
        "    </Test>\n";

    expected +=
        "    <Test id=\"6\">\n"
        "      <Name>skipped::tr6</Name>\n"
        "    </Test>\n";

    expected +=
        "  </SuccessfulTests>\n"
        "  <Statistics>\n"
        "    <Tests>8</Tests>\n"
        "    <FailuresTotal>6</FailuresTotal>\n"
        "    <Errors>2</Errors>\n"
        "    <Failures>4</Failures>\n"
        "  </Statistics>\n"
        "</TestRun>\n";

    std::string actual = ss.str();

    ensure(!repo.all_ok());
    ensure_equals( actual.begin(), actual.end(), expected.begin(), expected.end() );
}

template<>
template<>
void object::test<4>()
{
    ensure_equals( cppunit_reporter::encode("<"),  "&lt;" );
    ensure_equals( cppunit_reporter::encode(">"),  "&gt;" );
    ensure_equals( cppunit_reporter::encode("&"),  "&amp;" );
    ensure_equals( cppunit_reporter::encode("\""), "&quot;" );
    ensure_equals( cppunit_reporter::encode("\'"), "&apos;" );

    ensure_equals( cppunit_reporter::encode("<\'\">\"a&"), "&lt;&apos;&quot;&gt;&quot;a&amp;" );

}

template<>
template<>
void object::test<5>()
{
    set_test_name("tests handling errors when opening file");

    ensure_THROW( cppunit_reporter repo(".."), tut_error );
}

}

