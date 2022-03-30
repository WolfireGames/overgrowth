#include <tut/tut.hpp>
#include <memory>

using std::auto_ptr;

/**
 * This example test group tests std::auto_ptr implementation.
 * Tests are far from full, of course.
 */
namespace tut
{
/**
 * Struct which may contain test data members.
 * Test object (class that contains test methods)
 * will inherite from it, so each test method can
 * access members directly.
 *
 * Additionally, for each test, test object is re-created
 * using defaut constructor. Thus, any prepare work can be put
 * into default constructor.
 *
 * Finally, after each test, test object is destroyed independently
 * of test result, so any cleanup work should be located in destructor.
 */
struct auto_ptr_data
{
    /**
     * Type used to check scope lifetime of auto_ptr object.
     * Sets extern boolean value into true at constructor, and
     * to false at destructor.
     */
    bool exists;

    struct existing
    {
        bool& s_;
        existing(bool& s) : s_(s)
        {
            s_ = true;
        }

        ~existing()
        {
            s_ = false;
        }
    };

    auto_ptr_data(): exists(false) { }

    virtual ~auto_ptr_data() { }
};

/**
 * This group of declarations is just to register
 * test group in test-application-wide singleton.
 * Name of test group object (auto_ptr_group) shall
 * be unique in tut:: namespace. Alternatively, you
 * you may put it into anonymous namespace.
 */
typedef test_group<auto_ptr_data> tf;
typedef tf::object object;
tf auto_ptr_group("std::auto_ptr");

/**
 * Checks default constructor.
 */
template<>
template<>
void object::test<1>()
{
    auto_ptr<existing> ap;
    ensure(ap.get() == 0);
    ensure(ap.operator->() == 0);
}

/**
 * Checks constructor with object
 */
template<>
template<>
void object::test<2>()
{
    {
        auto_ptr<existing> ap(new existing(exists));
        ensure("get", ap.get() != 0);
        ensure_equals("constructed", exists, true);
    }
    // ptr left scope
    ensure_equals("destructed", exists, false);
}

/**
 * Checks operator -> and get()
 */
template<>
template<>
void object::test<3>()
{
    auto_ptr<existing> ap(new existing(exists));
    existing* p1 = ap.get();
    existing* p2 = ap.operator->();
    ensure("get equiv ->", p1 == p2);
    // ensure no losing ownership
    p1 = ap.get();
    ensure("still owner", p1 == p2);
}

/**
 * Checks release()
 */
template<>
template<>
void object::test<4>()
{
    {
        auto_ptr<existing> ap(new existing(exists));
        existing* p1 = ap.get();
        auto_ptr<existing> ap2(ap.release());
        ensure("same pointer", p1 == ap2.get());
        ensure("lost ownership", ap.get() == 0);
    }
    ensure("destructed", exists == false);
}

/**
 * Checks assignment.
 */
template<>
template<>
void object::test<5>()
{
    {
        auto_ptr<existing> ap(new existing(exists));
        existing* p1 = ap.get();
        auto_ptr<existing> ap2;
        ap2 = ap;
        ensure("same pointer", p1 == ap2.get());
        ensure("lost ownership", ap.get() == 0);
    }
    ensure("destructed", exists == false);
}

/**
 * Checks copy constructor.
 */
template<>
template<>
void object::test<6>()
{
    {
        auto_ptr<existing> ap(new existing(exists));
        existing* p1 = ap.get();
        auto_ptr<existing> ap2(ap);
        ensure("same pointer", p1 == ap2.get());
        ensure("lost ownership", ap.get() == 0);
    }
    ensure("destructed", exists == false);
}

}

