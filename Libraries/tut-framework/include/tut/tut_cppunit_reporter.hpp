
#ifndef TUT_CPPUNIT_REPORTER
#define TUT_CPPUNIT_REPORTER

#include <tut/tut.hpp>
#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <memory>

namespace tut
{

/**
 * CppUnit TUT reporter
 */
class cppunit_reporter : public tut::callback
{
    std::vector<tut::test_result> failed_tests_;
    std::vector<tut::test_result> passed_tests_;
    const std::string filename_;
    std::auto_ptr<std::ostream> stream_;


    cppunit_reporter(const cppunit_reporter &);
    cppunit_reporter &operator=(const cppunit_reporter &);

public:
    explicit cppunit_reporter(const std::string &filename = "testResult.xml")
        : failed_tests_(),
          passed_tests_(),
          filename_(filename),
          stream_(new std::ofstream(filename_.c_str()))
    {
        if (!stream_->good()) {
            throw tut_error("Cannot open output file `" + filename_ + "`");
        }
    }

    explicit cppunit_reporter(std::ostream &stream)
        : failed_tests_(),
          passed_tests_(),
          filename_(),
          stream_(&stream)
    {
    }

    ~cppunit_reporter()
    {
        if(filename_.empty())
        {
            stream_.release();
        }
    }

    void run_started()
    {
        failed_tests_.clear();
        passed_tests_.clear();
    }

    void test_completed(const tut::test_result& tr)
    {
        assert(tr.result != test_result::dummy );
        if ( (tr.result == test_result::ok) ||
             (tr.result == test_result::skipped) )
        {
            passed_tests_.push_back(tr);
        }
        else
        {
            failed_tests_.push_back(tr);
        }
    }

    void run_completed()
    {
        int errors = 0;
        int failures = 0;
        std::string failure_type;
        std::string failure_msg;

        *stream_ << "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>" << std::endl
                 << "<TestRun>" << std::endl;

        if (failed_tests_.size() > 0)
        {
            *stream_ << "  <FailedTests>" << std::endl;

            for (unsigned int i=0; i<failed_tests_.size(); i++)
            {
                switch (failed_tests_[i].result)
                {
                    case test_result::fail:
                        failure_type = "Assertion";
                        failure_msg  = "";
                        failures++;
                        break;
                    case test_result::ex:
                        failure_type = "Assertion";
                        failure_msg  = "Thrown exception: " + failed_tests_[i].exception_typeid + '\n';
                        failures++;
                        break;
                    case test_result::warn:
                        failure_type = "Assertion";
                        failure_msg  = "Destructor failed\n";
                        failures++;
                        break;
                    case test_result::term:
                        failure_type = "Error";
                        failure_msg  = "Test application terminated abnormally\n";
                        errors++;
                        break;
                    case test_result::ex_ctor:
                        failure_type = "Error";
                        failure_msg  = "Constructor has thrown an exception: " + failed_tests_[i].exception_typeid + '\n';
                        errors++;
                        break;
                    case test_result::rethrown:
                        failure_type = "Assertion";
                        failure_msg  = "Child failed\n";
                        failures++;
                        break;
                    default: // ok, skipped, dummy
                        failure_type = "Error";
                        failure_msg  = "Unknown test status, this should have never happened. "
                                       "You may just have found a bug in TUT, please report it immediately.\n";
                        errors++;
                        break;
                }

                *stream_ << "    <FailedTest id=\"" << failed_tests_[i].test << "\">" << std::endl
                            << "      <Name>" << encode(failed_tests_[i].group) + "::" + encode(failed_tests_[i].name) << "</Name>" << std::endl
                            << "      <FailureType>" << failure_type << "</FailureType>" << std::endl
                            << "      <Location>" << std::endl
                            << "        <File>Unknown</File>" << std::endl
                            << "        <Line>Unknown</Line>" << std::endl
                            << "      </Location>" << std::endl
                            << "      <Message>" << encode(failure_msg + failed_tests_[i].message) << "</Message>" << std::endl
                            << "    </FailedTest>" << std::endl;
            }

            *stream_ << "  </FailedTests>" << std::endl;
        }

        /* *********************** passed tests ***************************** */
        if (passed_tests_.size() > 0) {
            *stream_ << "  <SuccessfulTests>" << std::endl;

            for (unsigned int i=0; i<passed_tests_.size(); i++)
            {
                *stream_ << "    <Test id=\"" << passed_tests_[i].test << "\">" << std::endl
                            << "      <Name>" << encode(passed_tests_[i].group) + "::" + encode(passed_tests_[i].name) << "</Name>" << std::endl
                            << "    </Test>" << std::endl;
            }

            *stream_ << "  </SuccessfulTests>" << std::endl;
        }

        /* *********************** statistics ***************************** */
        *stream_ << "  <Statistics>" << std::endl
                    << "    <Tests>" << (failed_tests_.size() + passed_tests_.size()) << "</Tests>" << std::endl
                    << "    <FailuresTotal>" << failed_tests_.size() << "</FailuresTotal>" << std::endl
                    << "    <Errors>" << errors << "</Errors>" << std::endl
                    << "    <Failures>" << failures << "</Failures>" << std::endl
                    << "  </Statistics>" << std::endl;

        /* *********************** footer ***************************** */
        *stream_ << "</TestRun>" << std::endl;
    }

    virtual bool all_ok() const
    {
        return failed_tests_.empty();
    }

    /**
     * \brief Encodes text to XML
     * XML-reserved characters (e.g. "<") are encoded according to specification
     * @param text text to be encoded
     * @return encoded string
     */
    static std::string encode(const std::string & text)
    {
        std::string out;

        for (unsigned int i=0; i<text.length(); ++i) {
            char c = text[i];
            switch (c) {
                case '<':
                    out += "&lt;";
                    break;
                case '>':
                    out += "&gt;";
                    break;
                case '&':
                    out += "&amp;";
                    break;
                case '\'':
                    out += "&apos;";
                    break;
                case '"':
                    out += "&quot;";
                    break;
                default:
                    out += c;
            }
        }

        return out;
    }
};

}

#endif

