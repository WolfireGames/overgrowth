#include <tut/tut.hpp>

namespace tut
{
  /**
   * Testing ensure() method.
   */
  struct id_test
  {
      virtual ~id_test()
      {
      }
  };

  typedef test_group<id_test> tf;
  typedef tf::object object;
  tf id_test("test_id");

  /**
   * Checks positive ensure
   */
  template<>
  template<>
  void object::test<1>()
  {
      ensure_equals(get_test_id(), 1);
  }

  /**
   * Checks negative ensure
   */
  template<>
  template<>
  void object::test<3>()
  {
      ensure_equals(get_test_id(), 3);
  }
}
