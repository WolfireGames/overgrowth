#include <tut/tut.hpp>
#include <tut/tut_macros.hpp>
#include <tut/tut_xml_reporter.hpp>
#include <sstream>

using std::stringstream;

namespace tut
{

/**
 * Testing reporter.
 */
struct xml_reporter_test
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

    xml_reporter_test()
        : tr1("ok",       1, "tr1", test_result::ok),
          tr2("fail",     2, "tr2", test_result::fail,    "", "fail message"),
          tr3("fail",     3, "tr3", test_result::ex,      "exception", "ex message"),
          tr4("warn",     4, "tr4", test_result::warn,    "", "warn message"),
          tr5("term",     5, "tr5", test_result::term,    "", "term message"),
          tr6("ok"     ,  6, "tr6", test_result::skipped, "", "skipped message"),
          tr7("ctor",     7, "tr7", test_result::ex_ctor, "exception", "ex_ctor message"),
          tr8("rethrown", 8, "tr8", test_result::rethrown, "exception", "rethrown message"),
          filename("xml_reporter.log")
    {
    }

    virtual ~xml_reporter_test()
    {
        remove(filename.c_str());
    }
};

typedef test_group<xml_reporter_test> tg;
typedef tg::object object;
tg xml_reporter_test("xml reporter");

template<>
template<>
void object::test<1>()
{
    set_test_name("tests empty run report to the stream");

    std::stringstream ss;
    xml_reporter repo(ss);

    repo.run_started();
    repo.run_completed();

    std::string expected =
        "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n"
        "<testsuites>\n"
        "</testsuites>\n";

    std::string actual = ss.str();

    ensure(repo.all_ok());
    ensure_equals( actual.begin(), actual.end(), expected.begin(), expected.end() );
}

template<>
template<>
void object::test<2>()
{
    std::stringstream ss;
    xml_reporter repo(ss);

    ensure_equals("ok count", repo.ok_count, 0);
    ensure_equals("fail count", repo.failures_count, 0);
    ensure_equals("ex count", repo.exceptions_count, 0);
    ensure_equals("warn count", repo.warnings_count, 0);
    ensure_equals("term count", repo.terminations_count, 0);

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

    ensure_equals("ok count", repo.ok_count, 1+6); // 'skipped' means 'ok'
    ensure_equals("fail count", repo.failures_count, 2);
    ensure_equals("ex count", repo.exceptions_count, 3);
    ensure_equals("warn count", repo.warnings_count, 4);
    ensure_equals("term count", repo.terminations_count, 5);
    ensure(!repo.all_ok());
}

template<>
template<>
void object::test<3>()
{
    std::stringstream ss;
    xml_reporter repo(ss);

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
    std::stringstream ss;
    xml_reporter repo(ss);

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

    repo.run_started();
    repo.test_completed(tr1);
    repo.test_completed(tr7);
    repo.run_completed();
    ensure(!repo.all_ok());

    repo.run_started();
    repo.test_completed(tr1);
    repo.test_completed(tr8);
    repo.run_completed();
    ensure(!repo.all_ok());
}

template<>
template<>
void object::test<5>()
{
    set_test_name("tests empty run report to a file");

    {
        std::ifstream t(filename.c_str());
        ensure_equals( "File "+filename+" exists, remove it before running the test", t.good(), false);
    }
    {
        xml_reporter repo(filename);

        repo.run_started();
        repo.run_completed();

        std::string expected =
            "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n"
            "<testsuites>\n"
            "</testsuites>\n";

        std::ifstream file(filename.c_str());
        std::string actual;
        std::copy( std::istreambuf_iterator<char>(file.rdbuf()), std::istreambuf_iterator<char>(), std::back_inserter(actual) );

        ensure(repo.all_ok());
        ensure_equals( actual.begin(), actual.end(), expected.begin(), expected.end() );
    }
}

template<>
template<>
void object::test<6>()
{
    set_test_name("tests handling errors when opening file");

    ensure_THROW( xml_reporter repo(".."), tut_error );
}

template<>
template<>
void object::test<7>()
{
    std::stringstream ss;
    xml_reporter repo(ss);

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
        "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n"
        "<testsuites>\n";

    expected +=
        "  <testsuite errors=\"0\" failures=\"1\" tests=\"1\" name=\"ctor\">\n"
        "    <testcase classname=\"ctor\" name=\"tr7\">\n"
        "      <failure message=\"Constructor has thrown an exception: exception.\nex_ctor message\" type=\"Assertion\">Constructor has thrown an exception: exception.\nex_ctor message</failure>\n"
        "    </testcase>\n"
        "  </testsuite>\n";

    expected +=
        "  <testsuite errors=\"0\" failures=\"2\" tests=\"2\" name=\"fail\">\n"
        "    <testcase classname=\"fail\" name=\"tr2\">\n"
        "      <failure message=\"fail message\" type=\"Assertion\">fail message</failure>\n"
        "    </testcase>\n"
        "    <testcase classname=\"fail\" name=\"tr3\">\n"
        "      <failure message=\"Thrown exception: exception\nex message\" type=\"Assertion\">Thrown exception: exception\nex message</failure>\n"
        "    </testcase>\n"
        "  </testsuite>\n";

    expected +=
        "  <testsuite errors=\"0\" failures=\"0\" tests=\"2\" name=\"ok\">\n"
        "    <testcase classname=\"ok\" name=\"tr1\"/>\n"
        "    <testcase classname=\"ok\" name=\"tr6\"/>\n"
        "  </testsuite>\n";

    expected +=
        "  <testsuite errors=\"0\" failures=\"1\" tests=\"1\" name=\"rethrown\">\n"
        "    <testcase classname=\"rethrown\" name=\"tr8\">\n"
        "      <failure message=\"Child failed.\nrethrown message\" type=\"Assertion\">Child failed.\nrethrown message</failure>\n"
        "    </testcase>\n"
        "  </testsuite>\n";

    expected +=
        "  <testsuite errors=\"1\" failures=\"0\" tests=\"1\" name=\"term\">\n"
        "    <testcase classname=\"term\" name=\"tr5\">\n"
        "      <error message=\"Test application terminated abnormally.\nterm message\" type=\"Error\">Test application terminated abnormally.\nterm message</error>\n"
        "    </testcase>\n"
        "  </testsuite>\n";

    expected +=
        "  <testsuite errors=\"0\" failures=\"1\" tests=\"1\" name=\"warn\">\n"
        "    <testcase classname=\"warn\" name=\"tr4\">\n"
        "      <failure message=\"Destructor failed.\nwarn message\" type=\"Assertion\">Destructor failed.\nwarn message</failure>\n"
        "    </testcase>\n"
        "  </testsuite>\n";

    expected +=
        "</testsuites>\n";

    std::string actual = ss.str();

    ensure(!repo.all_ok());
    ensure_equals( actual.begin(), actual.end(), expected.begin(), expected.end() );
}

}

