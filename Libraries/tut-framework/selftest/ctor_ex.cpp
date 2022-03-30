#include <tut/tut.hpp>

#include <stdexcept>

using std::runtime_error;

/*
 * If the test ctor throws an exception, we shall terminate current group
 * execution: there is no reason to continue to run tests in that group as they
 * all going to fail and even if not their credibility is low anyway.
 */

namespace tut
{

class counter : public tut::callback
{

public:

    int cnt;

    counter()
        : cnt(0)
    {
    }

    void test_completed(const tut::test_result&)
    {
        cnt++;
    }
};

struct ctor_ex
{
    test_runner tr;
    struct dummy
    {
        dummy()
        {
            throw runtime_error("dummy has thrown an exception");
        }

        virtual ~dummy()
        {
        }
    };

    typedef test_group<dummy> tf;
    typedef tf::object object;
    tf factory;

    ctor_ex()
        : tr(),
          factory("internal", tr)
    {
    }

    virtual ~ctor_ex()
    {
    }
}
;

struct ctor_ex2
{
    test_runner tr;
    static int cnt;
    struct dummy
    {
        dummy()
        {
            if (cnt++ == 1)
            {
                throw runtime_error("dummy has thrown an exception");
            }
        }

        virtual ~dummy()
        {
        }
    };

    typedef test_group<dummy> tf;
    typedef tf::object object;
    tf factory;

    ctor_ex2()
        : tr(),
          factory("internal 2", tr)
    {
    }

    virtual ~ctor_ex2()
    {
    }
};

int ctor_ex2::cnt = 0;

typedef test_group<ctor_ex> tf;
typedef tf::object object;
tf ctor_ex("exceptions at ctor");

typedef test_group<ctor_ex2> tf2;
typedef tf2::object object2;
tf2 ctor_ex2("exceptions at ctor 2");

#if defined(TUT_USE_POSIX)
struct ctor_ex_posix
{
    test_runner tr;
    static int cnt;
    struct dummy : public tut_posix<dummy>
    {
        dummy()
        {
            if (cnt++ == 1)
            {
                // this throws
                fork();
            }
        }
    };

    typedef test_group<dummy> tf;
    typedef tf::object object;
    tf factory;

    ctor_ex_posix()
        : tr(),
          factory("internal posix", tr)
    {
    }

    virtual ~ctor_ex_posix()
    {
    }
};

int ctor_ex_posix::cnt = 0;

typedef test_group<ctor_ex_posix> tf_posix;
typedef tf_posix::object object_posix;
tf_posix ctor_ex_posix("exceptions at ctor posix");
#endif

template<>
template<>
void object::test<1>()
{
    counter cnt;
    tr.set_callback(&cnt);
    tr.run_tests("internal");
    ensure_equals("called only once", cnt.cnt, 1);
}

template<>
template<>
void object::test<2>()
{
    counter cnt;
    tr.set_callback(&cnt);
    tr.run_tests();
    ensure_equals("called only once", cnt.cnt, 1);
}


template<>
template<>
void ctor_ex2::object::test<1>()
{
}

template<>
template<>
void ctor_ex2::object::test<2>()
{
}

template<>
template<>
void ctor_ex2::object::test<3>()
{
}

template<>
template<>
void object2::test<1>()
{
    counter cnt;
    tr.set_callback(&cnt);
    tr.run_tests("internal 2");
    ensure_equals("called only twice", cnt.cnt, 2);
}

#if defined(TUT_USE_POSIX)
template<>
template<>
void object_posix::test<1>()
{
    counter cnt;
    tr.set_callback(&cnt);
    tr.run_tests("internal posix");
    ensure_equals("called only twice", cnt.cnt, 2);
}

template<>
template<>
void ctor_ex_posix::object::test<1>()
{
}

template<>
template<>
void ctor_ex_posix::object::test<2>()
{
}

template<>
template<>
void ctor_ex_posix::object::test<3>()
{
}
#endif

}

