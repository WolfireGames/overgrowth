#include <tut/tut.hpp>
#include <tut/tut_macros.hpp>
#include <string>
#include <sstream>
#include <stdexcept>

using std::string;
using std::ostringstream;
using std::runtime_error;


namespace tut
{
  /**
   * Testing ensure() method.
   */
  struct ensure_test
  {
      virtual ~ensure_test()
      {
      }
  };

  typedef test_group<ensure_test> tf;
  typedef tf::object object;
  tf ensure_test("ensure");

  /**
   * Checks positive ensure
   */
  template<>
  template<>
  void object::test<1>()
  {
    set_test_name("checks positive ensure");

    ensure("OK", 1==1);
    ensure(1==1);
  }

  /**
   * Checks negative ensure
   */
  template<>
  template<>
  void object::test<2>()
  {
    set_test_name("checks negative ensure");

    try
    {
      ensure("ENSURE", 1==2);

      // we cannot relay on fail here; we haven't tested it yet ;)
      throw runtime_error("passed below");
    }
    catch (const failure& ex)
    {
      string msg = ex.what();
      if(msg.find("ENSURE") == string::npos )
      {
        throw runtime_error("ex.what has no ENSURE");
      }
    }

    try
    {
      ensure(1 == 2);
      throw runtime_error("passed below");
    }
    catch (const failure&)
    {
    }
  }

  /**
   * Checks ensure with various "constructed" messages
   */
  template<>
  template<>
  void object::test<3>()
  {
    set_test_name("checks ensure with const char*");

    const char* ok1 = "OK";
    ensure(ok1, 1 == 1);
  }

#pragma GCC diagnostic ignored "-Wwrite-strings"
  template<>
  template<>
  void object::test<4>()
  {
    set_test_name("checks ensure with char*");

    char* ok2 = "OK";
    ensure(ok2, 1 == 1);
  }
#pragma GCC diagnostic error "-Wwrite-strings"

  template<>
  template<>
  void object::test<5>()
  {
    set_test_name("checks ensure with std::string");

    string msg = "OK";
    ensure(msg, 1 == 1);
  }

  template<>
  template<>
  void object::test<6>()
  {
    set_test_name("checks ensure with std::ostringstream");

    ostringstream oss;
    oss << "OK";
    ensure(oss.str(), 1 == 1);
  }
}
