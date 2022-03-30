#ifndef TUT_XML_REPORTER
#define TUT_XML_REPORTER
#include <tut/tut_config.hpp>
#include <tut/tut.hpp>
#include <tut/tut_cppunit_reporter.hpp>
#include <cassert>
#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>

#if defined(TUT_USE_POSIX)
#include <sys/types.h>
#include <unistd.h>
#endif

namespace tut
{

/**
 * \brief JUnit XML TUT reporter
 * @author Lukasz Maszczynski, NSN
 * @date 11/07/2008
 */
class xml_reporter : public tut::callback
{
    typedef std::vector<tut::test_result> TestResults;
    typedef std::map<std::string, TestResults> TestGroups;

    TestGroups all_tests_; /// holds all test results
    const std::string filename_; /// filename base
    std::auto_ptr<std::ostream> stream_;

    /**
     * \brief Builds "testcase" XML entity with given parameters
     * Builds \<testcase\> entity according to given parameters. \<testcase\>-s are part of \<testsuite\>.
     * @param tr test result to be used as source data
     * @param failure_type type of failure to be reported ("Assertion" or "Error", empty if test passed)
     * @param failure_msg failure message to be reported (empty, if test passed)
     * @return string with \<testcase\> entity
     */
    std::string xml_build_testcase(const tut::test_result & tr, const std::string & failure_type,
                                           const std::string & failure_msg, int pid = 0)
    {
        using std::endl;
        using std::string;

        std::ostringstream out;

        if ( (tr.result == test_result::ok) ||
             (tr.result == test_result::skipped) )
        {
            out << "    <testcase classname=\"" << cppunit_reporter::encode(tr.group) << "\" name=\"" << cppunit_reporter::encode(tr.name) << "\"/>";
        }
        else
        {
            string err_msg = cppunit_reporter::encode(failure_msg + tr.message);

            string tag; // determines tag name: "failure" or "error"
            if ( tr.result == test_result::fail || tr.result == test_result::warn ||
                 tr.result == test_result::ex || tr.result == test_result::ex_ctor || tr.result == test_result::rethrown )
            {
                tag = "failure";
            }
            else
            {
                tag = "error";
            }

            out << "    <testcase classname=\"" << cppunit_reporter::encode(tr.group) << "\" name=\"" << cppunit_reporter::encode(tr.name) << "\">" << endl;
            out << "      <" << tag << " message=\"" << err_msg << "\"" << " type=\"" << failure_type << "\"";
#if defined(TUT_USE_POSIX)
            if(pid != getpid())
            {
                out << " child=\"" << pid << "\"";
            }
#else
            (void)pid;
#endif
            out << ">" << err_msg << "</" << tag << ">" << endl;
            out << "    </testcase>";
        }

        return out.str();
    }

    /**
     * \brief Builds "testsuite" XML entity
     * Builds \<testsuite\> XML entity according to given parameters.
     * @param errors number of errors to be reported
     * @param failures number of failures to be reported
     * @param total total number of tests to be reported
     * @param name test suite name
     * @param testcases cppunit_reporter::encoded XML string containing testcases
     * @return string with \<testsuite\> entity
     */
    std::string xml_build_testsuite(int errors, int failures, int total,
                                            const std::string & name, const std::string & testcases)
    {
        std::ostringstream out;

        out << "  <testsuite errors=\"" << errors << "\" failures=\"" << failures << "\" tests=\"" << total << "\" name=\"" << cppunit_reporter::encode(name) << "\">" << std::endl;
        out << testcases;
        out << "  </testsuite>";

        return out.str();
    }

public:
    int ok_count;           /// number of passed tests
    int exceptions_count;   /// number of tests that threw exceptions
    int failures_count;     /// number of tests that failed
    int terminations_count; /// number of tests that would terminate
    int warnings_count;     /// number of tests where destructors threw an exception

    /**
     * \brief Default constructor
     * @param filename base filename
     */
    xml_reporter(const std::string & filename)
        : all_tests_(),
          filename_(filename),
          stream_(new std::ofstream(filename_.c_str())),
          ok_count(0),
          exceptions_count(0),
          failures_count(0),
          terminations_count(0),
          warnings_count(0)
    {
        if (!stream_->good()) {
            throw tut_error("Cannot open output file `" + filename_ + "`");
        }
    }

    xml_reporter(std::ostream & stream)
        : all_tests_(),
          filename_(),
          stream_(&stream),
          ok_count(0),
          exceptions_count(0),
          failures_count(0),
          terminations_count(0),
          warnings_count(0)
    {
    }

    ~xml_reporter()
    {
        if(filename_.empty())
        {
            stream_.release();
        }
    }

    /**
     * \brief Callback function
     * This function is called before the first test is executed. It initializes counters.
     */
    virtual void run_started()
    {
        ok_count = 0;
        exceptions_count = 0;
        failures_count = 0;
        terminations_count = 0;
        warnings_count = 0;
        all_tests_.clear();
    }

    /**
     * \brief Callback function
     * This function is called when test completes. Counters are updated here, and test results stored.
     */
    virtual void test_completed(const tut::test_result& tr)
    {
        // update global statistics
        switch (tr.result) {
            case test_result::ok:
            case test_result::skipped:
                ok_count++;
                break;
            case test_result::fail:
            case test_result::rethrown:
                failures_count++;
                break;
            case test_result::ex:
            case test_result::ex_ctor:
                exceptions_count++;
                break;
            case test_result::warn:
                warnings_count++;
                break;
            case test_result::term:
                terminations_count++;
                break;
            case tut::test_result::dummy:
                assert(!"Should never be called");
        } // switch

        // add test result to results table
        all_tests_[tr.group].push_back(tr);
    }

    /**
     * \brief Callback function
     * This function is called when all tests are completed. It generates XML output
     * to file(s). File name base can be set with constructor.
     */
    virtual void run_completed()
    {
        /* *********************** header ***************************** */
        *stream_ << "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>" << std::endl;
        *stream_ << "<testsuites>" << std::endl;

        // iterate over all test groups
        for (TestGroups::const_iterator tgi = all_tests_.begin(); tgi != all_tests_.end(); ++tgi)
        {
            /* per-group statistics */
            int passed = 0;         // passed in single group
            int exceptions = 0;     // exceptions in single group
            int failures = 0;       // failures in single group
            int terminations = 0;   // terminations in single group
            int warnings = 0;       // warnings in single group
            int errors = 0;         // errors in single group


            // output is written to string stream buffer, because JUnit format <testsuite> tag
            // contains statistics, which aren't known yet
            std::ostringstream out;

            // iterate over all test cases in the current test group
            const TestResults &results = tgi->second;
            for (TestResults::const_iterator tri = results.begin(); tri != results.end(); ++tri)
            {
                std::string failure_type;    // string describing the failure type
                std::string failure_msg;     // a string with failure message

                switch (tri->result)
                {
                    case test_result::ok:
                    case test_result::skipped:
                        passed++;
                        break;
                    case test_result::fail:
                        failure_type = "Assertion";
                        failure_msg  = "";
                        failures++;
                        break;
                    case test_result::ex:
                        failure_type = "Assertion";
                        failure_msg  = "Thrown exception: " + tri->exception_typeid + '\n';
                        exceptions++;
                        break;
                    case test_result::warn:
                        failure_type = "Assertion";
                        failure_msg  = "Destructor failed.\n";
                        warnings++;
                        break;
                    case test_result::term:
                        failure_type = "Error";
                        failure_msg  = "Test application terminated abnormally.\n";
                        terminations++;
                        break;
                    case test_result::ex_ctor:
                        failure_type = "Assertion";
                        failure_msg  = "Constructor has thrown an exception: " + tri->exception_typeid + ".\n";
                        exceptions++;
                        break;
                    case test_result::rethrown:
                        failure_type = "Assertion";
                        failure_msg  = "Child failed.\n";
                        failures++;
                        break;
                    default:
                        failure_type = "Error";
                        failure_msg  = "Unknown test status, this should have never happened. "
                                       "You may just have found a bug in TUT, please report it immediately.\n";
                        errors++;
                        break;
                } // switch

#if defined(TUT_USE_POSIX)
                out << xml_build_testcase(*tri, failure_type, failure_msg, tri->pid) << std::endl;
#else
                out << xml_build_testcase(*tri, failure_type, failure_msg) << std::endl;
#endif
            } // iterate over all test cases

            // calculate per-group statistics
            int stat_errors = terminations + errors;
            int stat_failures = failures + warnings + exceptions;
            int stat_all = stat_errors + stat_failures + passed;

            *stream_ << xml_build_testsuite(stat_errors, stat_failures, stat_all, (*tgi).first/* name */, out.str()/* testcases */) << std::endl;
        } // iterate over all test groups

        *stream_ << "</testsuites>" << std::endl;
    }

    /**
     * \brief Returns true, if all tests passed
     */
    virtual bool all_ok() const
    {
        return ( (terminations_count + failures_count + warnings_count + exceptions_count) == 0);
    };
};

}

#endif
