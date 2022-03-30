//-----------------------------------------------------------------------------
//           Name: rand.cpp
//      Developer: Wolfire Games LLC
//         Author: Max Danielsson
//    Description: This is a threadsafe variant of pcg rand(), which doesn't 
//                 require a local reseed variable.
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#include "rand.h"

#include <Utility/pcg_basic.h>
#include <Internal/integer.h>

#include <cstdlib>
#include <mutex>

static std::mutex mutex;
static unsigned int reseed;
static pcg32_random_t seedr;

void rand_ts_seed(unsigned int seed)
{
    mutex.lock();
    pcg32_srandom_r(&seedr,seed,seed);
    mutex.unlock();
}

uint32_t rand_ts()
{
    uint32_t v;
    mutex.lock();
    v = pcg32_random_r(&seedr);
    mutex.unlock();
    return v;
}
