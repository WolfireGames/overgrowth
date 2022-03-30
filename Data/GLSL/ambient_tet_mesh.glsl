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

float Determinant4x4( vec4 v0,
                      vec4 v1,
                      vec4 v2,
                      vec4 v3 )
{
    float det = v0[3]*v1[2]*v2[1]*v3[0] - v0[2]*v1[3]*v2[1]*v3[0] -
                v0[3]*v1[1]*v2[2]*v3[0] + v0[1]*v1[3]*v2[2]*v3[0] +

                v0[2]*v1[1]*v2[3]*v3[0] - v0[1]*v1[2]*v2[3]*v3[0] -
                v0[3]*v1[2]*v2[0]*v3[1] + v0[2]*v1[3]*v2[0]*v3[1] +

                v0[3]*v1[0]*v2[2]*v3[1] - v0[0]*v1[3]*v2[2]*v3[1] -
                v0[2]*v1[0]*v2[3]*v3[1] + v0[0]*v1[2]*v2[3]*v3[1] +

                v0[3]*v1[1]*v2[0]*v3[2] - v0[1]*v1[3]*v2[0]*v3[2] -
                v0[3]*v1[0]*v2[1]*v3[2] + v0[0]*v1[3]*v2[1]*v3[2] +

                v0[1]*v1[0]*v2[3]*v3[2] - v0[0]*v1[1]*v2[3]*v3[2] -
                v0[2]*v1[1]*v2[0]*v3[3] + v0[1]*v1[2]*v2[0]*v3[3] +

                v0[2]*v1[0]*v2[1]*v3[3] - v0[0]*v1[2]*v2[1]*v3[3] -
                v0[1]*v1[0]*v2[2]*v3[3] + v0[0]*v1[1]*v2[2]*v3[3];
    return det;
}

vec4 GetBarycentricCoordinate( vec3 v0_,
                               vec3 v1_,
                               vec3 v2_,
                               vec3 v3_,
                               vec3 p0_)
{
    vec4 v0 = vec4(v0_, 1.0);
    vec4 v1 = vec4(v1_, 1.0);
    vec4 v2 = vec4(v2_, 1.0);
    vec4 v3 = vec4(v3_, 1.0);
    vec4 p0 = vec4(p0_, 1.0);
    vec4 barycentricCoord;
    float det0 = Determinant4x4(v0, v1, v2, v3);
    float det1 = Determinant4x4(p0, v1, v2, v3);
    float det2 = Determinant4x4(v0, p0, v2, v3);
    float det3 = Determinant4x4(v0, v1, p0, v3);
    float det4 = Determinant4x4(v0, v1, v2, p0);
    barycentricCoord[0] = (det1/det0);
    barycentricCoord[1] = (det2/det0);
    barycentricCoord[2] = (det3/det0);
    barycentricCoord[3] = (det4/det0);
    return barycentricCoord;
}

// tetrahedron points (128)
// tetrahedron neighbor ids (128)

// tetrahedron render info: (128*3)
//    1 bit per point (is valid point?)
//    red 5*4*6
//    green 5*4*6
//    blue 5*4*6

const uint NULL_TET = 4294967295u;

bool GetAmbientCube(in vec3 pos, int num_tetrahedra, in usamplerBuffer tet_buf, out vec3 ambient_cube_color[6], uint guess) {
    if(num_tetrahedra == 0){
        return false;
    }

    uint last_guess = guess;
    uint tet_guess = guess;
    int num_guess = 0;
    const int kMaxGuess = 1000;

    while(tet_guess != NULL_TET && num_guess < kMaxGuess){
        ++num_guess;
        int index = int(tet_guess)*8;
        bool reject = false;
        uvec4 tet_point_bits = texelFetch(tet_buf, index);
        uvec4 tet_point_bits2 = texelFetch(tet_buf, index+1);
        // 128 bits
        // 16 bits per axis for first point (16*3 = 48 bits)
        // 8 bits per axis for delta to other points (8*3*3 = 72 bits)
        const float scalar = 10.0;
        vec3 points[4];
        points[0][0] = (float(tet_point_bits[0] / 65536u) - 32767.0) / scalar;
        points[0][1] = (float(tet_point_bits[0] % 65536u) - 32767.0) / scalar;
        points[0][2] = (float(tet_point_bits[1] / 65536u) - 32767.0) / scalar;

        points[1][0] = (float(tet_point_bits[1] % 65536u) - 32767.0) / scalar;
        points[1][1] = (float(tet_point_bits[2] / 65536u) - 32767.0) / scalar;
        points[1][2] = (float(tet_point_bits[2] % 65536u) - 32767.0) / scalar;

        points[2][0] = (float(tet_point_bits[3] / 65536u) - 32767.0) / scalar;
        points[2][1] = (float(tet_point_bits[3] % 65536u) - 32767.0) / scalar;
        points[2][2] = (float(tet_point_bits2[0] / 65536u) - 32767.0) / scalar;

        points[3][0] = (float(tet_point_bits2[0] % 65536u) - 32767.0) / scalar;
        points[3][1] = (float(tet_point_bits2[1] / 65536u) - 32767.0) / scalar;
        points[3][2] = (float(tet_point_bits2[1] % 65536u) - 32767.0) / scalar;

        for(int i=0; i<6; ++i){
            ambient_cube_color[i] = vec3(0.0);
        }

        vec4 bary_coords =
            GetBarycentricCoordinate(points[0], points[1],
                                     points[2], points[3],
                                     pos);

        uvec4 neighbors = texelFetch(tet_buf, index+2);
        bool val = false;

        if(num_guess < kMaxGuess){
            if(bary_coords[0] < 0.0){
                if(last_guess != neighbors[0]){
                    last_guess = tet_guess;
                    tet_guess = neighbors[0];
                    val = true;
                }
            } else if(!val && bary_coords[1] < 0.0){
                if(last_guess != neighbors[1]){
                    last_guess = tet_guess;
                    tet_guess = neighbors[1];
                    val = true;
                }
            } else if(!val && bary_coords[2] < 0.0){
                if(last_guess != neighbors[2]){
                    last_guess = tet_guess;
                    tet_guess = neighbors[2];
                    val = true;
                }
            } else if(!val && bary_coords[3] < 0.0){
                if(last_guess != neighbors[3]){
                    last_guess = tet_guess;
                    tet_guess = neighbors[3];
                    val = true;
                }
            }
        }

        if(!val){
            // Reduce/eliminate contribution from probes inside of walls
            // This loop is unrolled because otherwise there is a problem on Intel cards
            if((tet_point_bits2[2]&1u) == 0u){
                bary_coords[0] *= 0.01;
            }

            if((tet_point_bits2[2]&2u) == 0u){
                bary_coords[1] *= 0.01;
            }

            if((tet_point_bits2[2]&4u) == 0u){
                bary_coords[2] *= 0.01;
            }

            if((tet_point_bits2[2]&8u) == 0u){
                bary_coords[3] *= 0.01;
            }

            float total_bary_coords = bary_coords[0] + bary_coords[1] + bary_coords[2] + bary_coords[3];
            bary_coords /= total_bary_coords;

            uvec4 color[5];
            color[0] = texelFetch(tet_buf, index+3);
            color[1] = texelFetch(tet_buf, index+4);
            color[2] = texelFetch(tet_buf, index+5);
            color[3] = texelFetch(tet_buf, index+6);
            color[4] = texelFetch(tet_buf, index+7);

            uint offset=0u;

            for(int point=0; point<4; ++point){
                for(int face=0; face<6; ++face){
                    for(int channel=0; channel<3; ++channel){
                        int array_index = int(offset/128u);
                        int channel_index = int((offset % 128u) / 32u);
                        uint val = (color[array_index][channel_index] >> (24u-(offset%32u))) % 256u;
                        ambient_cube_color[face][channel] += float(val) / 255.0 * bary_coords[point] * 4.0;
                        offset += 8u;
                    }
                }
            }

            return true;
        }
    }

    return false;
}

vec3 SampleAmbientCube(in vec3 ambient_cube_color[6], in vec3 vec){
    float sum = abs(vec[0]) + abs(vec[1]) + abs(vec[2]);
    vec3 temp_vert = vec / vec3(sum);
    vec3 total_cube = vec3(0.0);
    total_cube += ambient_cube_color[0+int(temp_vert.x<0)] * vec3(abs(temp_vert.x));
    total_cube += ambient_cube_color[2+int(temp_vert.y<0)] * vec3(abs(temp_vert.y));
    total_cube += ambient_cube_color[4+int(temp_vert.z<0)] * vec3(abs(temp_vert.z));
    return total_cube;
}
