// This file is meant to be included from another script file

// We're allowed to include files in a circular manner, the
// application will include each file only once anyway
#include "script.as"

void includedFunction()
{
    print("I'm now in includedFunction()\n");
}
