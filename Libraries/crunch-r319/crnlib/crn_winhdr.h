#pragma once

#ifndef WIN32
#error Should not get here
#endif

// Why is crunch defining this? 0x500 is pre-XP
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x500
//#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "windows.h"
