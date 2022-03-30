#include <tut/tut.hpp>

namespace tut
{
  /**
   * Testing ensure() method.
   */
  struct ensure_not_test
  {
      virtual ~ensure_not_test()
      {
      }
  };

  typedef test_group<ensure_not_test> tf;
  typedef tf::object object;
  tf ensure_not_test("ensure_not");

  /**
   * Checks ensure_not
   */
  template<>
  template<>
  void object::test<1>()
  {
    set_test_name("checks ensure_not");

    ensure_not("ok", 1==2);
    ensure_not(1==2);
  }
}
