#include <tut/tut.hpp>
#include <stdexcept>

#include "shared_ptr.h"

using std::runtime_error;

/**
 * This example test group tests shared_ptr implementation
 * as tutorial example for TUT framework.
 */
namespace tut
{

struct shared_ptr_data
{
    bool keepee_exists;

    struct keepee
    {
        bool& s_;
        keepee(bool& s) : s_(s)
        {
            s_ = true;
        }

        ~keepee()
        {
            s_ = false;
        }
    };

    shared_ptr_data()
        : keepee_exists(false)
    {
    }

    virtual ~shared_ptr_data()
    {
    }
};

typedef test_group<shared_ptr_data> tg;
typedef tg::object object;
tg shared_ptr_group("shared_ptr");

// =================================================
// Constructors
// =================================================

/**
 * Checks default constructor.
 */
template<>
template<>
void object::test<1>()
{
    shared_ptr<keepee> ap;
    ensure(ap.get() == 0);
}

/**
 * Checks constructor with object.
 */
template<>
template<>
void object::test<2>()
{
    {
        keepee* keepee_ = new keepee(keepee_exists);
        shared_ptr<keepee> ap(keepee_);
        ensure("get", ap.get() == keepee_);
        ensure_equals("constructed", keepee_exists, true);
    }
    // ptr left scope
    ensure_equals("destructed", keepee_exists, false);
}

/**
 * Checks constructor with null object.
 */
template<>
template<>
void object::test<3>()
{
    shared_ptr<keepee> ap(0);
    ensure("get", ap.get() == 0);
}

/**
 * Checks constructor with another shared_ptr with no object.
 */
template<>
template<>
void object::test<4>()
{
    shared_ptr<keepee> sp1;
    ensure_equals("sp1.count:1", sp1.count(), 1);
    shared_ptr<keepee> sp2(sp1);
    ensure_equals("sp1.count:2", sp1.count(), 2);
    ensure_equals("sp2.count", sp2.count(), 2);
    ensure(sp2.get() == 0);
}

/**
 * Checks constructor with another shared_ptr with object.
 */
template<>
template<>
void object::test<5>()
{
    {
        keepee* keepee_ = new keepee(keepee_exists);
        shared_ptr<keepee> sp1(keepee_);
        shared_ptr<keepee> sp2(sp1);
        ensure("get", sp1.get() == keepee_);
        ensure("get", sp2.get() == keepee_);
        ensure("cnt", sp1.count() == 2);
    }
    // ptr left scope
    ensure_equals("destructed", keepee_exists, false);
}

// =================================================
// Assignment operators
// =================================================

/**
 * Checks assignment with null object.
 */
template<>
template<>
void object::test<10>()
{
    keepee* p = 0;
    shared_ptr<keepee> sp;
    sp = p;
    ensure("get", sp.get() == 0);
    ensure("cnt", sp.count() == 1);
}

/**
 * Checks assignment with non-null object.
 */
template<>
template<>
void object::test<11>()
{
    keepee* p = new keepee(keepee_exists);
    shared_ptr<keepee> sp;
    sp = p;
    ensure("get", sp.get() == p);
    ensure("cnt", sp.count() == 1);
}

/**
 * Checks assignment with shared_ptr with null object.
 */
template<>
template<>
void object::test<12>()
{
    shared_ptr<keepee> sp1(0);
    shared_ptr<keepee> sp2;
    sp2 = sp1;
    ensure("get", sp1.get() == 0);
    ensure("get", sp2.get() == 0);
    ensure("cnt", sp1.count() == 2);
}

/**
 * Checks assignment with shared_ptr with non-null object.
 */
template<>
template<>
void object::test<13>()
{
    {
        shared_ptr<keepee> sp1(new keepee(keepee_exists));
        shared_ptr<keepee> sp2;
        sp2 = sp1;
        ensure("get", sp1.get() != 0);
        ensure("get", sp2.get() != 0);
        ensure("cnt", sp1.count() == 2);
    }
    ensure_equals("destructed", keepee_exists, false);
}

/**
 * Checks assignment with itself.
 */
template<>
template<>
void object::test<14>()
{
    shared_ptr<keepee> sp1(new keepee(keepee_exists));
    sp1 = sp1;
    ensure("get", sp1.get() != 0);
    ensure("cnt", sp1.count() == 1);
    ensure_equals("not destructed", keepee_exists, true);
}


// =================================================
// Passing ownership
// =================================================

/**
 * Checks passing ownership via assignment.
 */
template<>
template<>
void object::test<20>()
{
    bool flag1;
    bool flag2;
    shared_ptr<keepee> sp1(new keepee(flag1));
    shared_ptr<keepee> sp2(new keepee(flag2));
    ensure_equals("flag1=true", flag1, true);
    ensure_equals("flag2=true", flag2, true);

    sp1 = sp2;
    ensure_equals("flag1=false", flag1, false);
    ensure_equals("flag2=true", flag2, true);
    ensure_equals("cnt=2", sp1.count(), 2);

    sp2.reset();
    ensure_equals("flag2=true", flag2, true);
    ensure_equals("cnt=1", sp2.count(), 1);
}

/**
 * Checks operator -&gt; throws instead of returning null.
 */
template<>
template<>
void object::test<21>()
{
    try
    {
        shared_ptr<keepee> sp;
        sp->s_ = !sp->s_;
        fail("exception expected");
    }
    catch (const runtime_error&)
    {
        // ok
    }
}

}

