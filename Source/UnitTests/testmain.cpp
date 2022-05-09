//-----------------------------------------------------------------------------
//           Name: testmain.cpp
//      Developer: Wolfire Games LLC
//    Description:
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
#include "testmain.h"

#include <tut/tut.hpp>
#include <tut/tut_reporter.hpp>

#include <iostream>

using std::cerr;
using std::endl;
using std::exception;

namespace tut {
test_runner_singleton runner;
}

int RunUnitTests() {
    tut::reporter reporter;
    tut::runner.get().set_callback(&reporter);

    tut::runner.get().run_tests();

    return !reporter.all_ok();
}
