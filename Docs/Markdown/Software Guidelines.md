---
format: Markdown
...

# List of pitfalls

C++ isn't the best language for safe programming. All object have default copy constuctors, algorithms executed from different threads aren't naturally separated, etc. This list is a non-exhaustive list of pitfalls when working in the Main Overgrowth code. Problems that won't give you a compiler error or warning, but which might give you unexpected segfaults, raceconditions or soft-locks.

 - SDL_GetTicks() is used from multiple threads at one. Therefore all calls have to go through the thread safe SDL_TS_GetTicks()

# C++ Format Guidelines and Observations (maybe future rules)

Classes that might seem copy-able or assignable but aren't should always have private copy constructors and assignment operators. This allows us to implement references counted copy-able object for things like VBOContainer without creating dangerours expectations for developers. To disable these functions the header needs these additional definitions.

    class NonAssignable {
    private:
        NonAssignable(NonAssignable const&);
        NonAssignable& operator=(NonAssignable const&);
    public:
        NonAssignable() {}
    };
