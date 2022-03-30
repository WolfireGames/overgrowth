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
mat3 transposeMat3(const mat3 matrix) {
    mat3 temp;
    temp[0][0] = matrix[0][0];
    temp[0][1] = matrix[1][0];
    temp[0][2] = matrix[2][0];
    temp[1][0] = matrix[0][1];
    temp[1][1] = matrix[1][1];
    temp[1][2] = matrix[2][1];
    temp[2][0] = matrix[0][2];
    temp[2][1] = matrix[1][2];
    temp[2][2] = matrix[2][2];
    return temp;
}

