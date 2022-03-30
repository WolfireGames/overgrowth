tut-framework
=============

TUT is a small and portable unit test framework for C++.

* TUT is very portable, no matter what compiler or OS you use.
* TUT consists of header files only. No libraries required, deployment has never been easier.
* Custom reporter interface allows to integrate TUT with virtually any IDE or tool in the world.
* Support for multi-process testing (testing deadlocks and timeouts is under way).
* TUT is free and distributed under a BSD-like license.
* Tests are organised into named test groups.
* Regression (all tests in the application), one-group or one-test execution.
* Pure C++, no macros!

TUT tests are easy to read and maintain. Here’s the simplest test file possible:

```cpp
#include <tut/tut.hpp>

namespace tut
{
    struct basic{};
    typedef test_group<basic> factory;
    typedef factory::object object;
}

namespace
{
    tut::factory tf("basic test");
}

namespace tut
{
    template<>
    template<>
    void object::test<1>()
    {
        ensure_equals("2+2=?", 2+2, 4);
    }
}
```
