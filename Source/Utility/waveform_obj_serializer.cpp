//-----------------------------------------------------------------------------
//           Name: waveform_obj_serializer.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "waveform_obj_serializer.h"

#include <fstream>

using std::endl;
using std::ofstream;

void SerializeToObj(const string& path, const vector<float>& vertices, const vector<uint32_t>& faces) {
    ofstream output(path, ofstream::out);

    for (int i = 0; i < vertices.size(); i += 3) {
        output << "v " << vertices[i] << " " << vertices[i + 1] << " " << vertices[i + 2] << endl;
    }

    for (int i = 0; i < faces.size(); i += 3) {
        output << "f " << faces[i] + 1 << " " << faces[i + 1] + 1 << " " << faces[i + 2] + 1 << endl;
    }

    output.close();
}
