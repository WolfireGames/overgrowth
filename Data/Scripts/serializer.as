//-----------------------------------------------------------------------------
//           Name: serializer.as
//      Developer: Wolfire Games LLC
//    Script Type:
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

class Serializer {
    array<uint8> buffer;
    bool open;

    Serializer() {
        open = true;
    }

    void PutBool(bool v) {
        if (!open) {
            Log(error, "Attempting to PutBool to closed Serializer!");
            return;
        }

        if (v) {
            buffer.insertLast(1);
        } else {
            buffer.insertLast(0);
        }
    }


    void PutUint(uint u) {
        if (!open) {
            Log(error, "Attempting to PutUint to closed Serializer!");
            return;
        }

        for (uint i = 0; i < 4; i++) {
            buffer.insertLast(u & 0xFF);
            u = u >> 8;
        }
    }


    void PutFloat(float f) {
        if (!open) {
            Log(error, "Attempting to PutFloat to closed Serializer!");
            return;
        }

        PutUint(fpToIEEE(f));
    }


    void PutVec3(vec3 v) {
        if (!open) {
            Log(error, "Attempting to PutFloat to closed Serializer!");
            return;
        }

        PutFloat(v.x);
        PutFloat(v.y);
        PutFloat(v.z);
    }

	void PutVec2(vec2 v) {
	    if (!open) {
            Log(error, "Attempting to PutFloat to closed Serializer!");
            return;
        }

		PutFloat(v.x);
		PutFloat(v.y);
	}


    void PutString(string str) {
        uint l = str.length();
        PutUint(l);
        for (uint i = 0; i < l; i++) {
            buffer.insertLast(str[i]);
        }
    }


    array<uint8> GetBuffer() {
        open = false;
        return buffer;
    }
};


class Deserializer {
    array<uint8> buffer;
    uint index;

    Deserializer(array<uint8> &buf) {
        buffer = buf;
        index = 0;
    }

    float GetFloat() {
        return fpFromIEEE(GetUint());
    }


    uint GetUint() {
        uint u = 0;
        for (uint i = 0; i < 4; i++) {
            uint b = buffer[index + i];
            u = u | (b << (8 * i));
        }
        index += 4;
        return u;
    }


    bool GetBool() {
        uint8 b = buffer[index];
        index++;
        if (b == 1) {
            return true;
        } else {
            return false;
        }
    }


    vec3 GetVec3() {
        vec3 v;
        v.x = GetFloat();
        v.y = GetFloat();
        v.z = GetFloat();
        return v;
    }

	vec2 GetVec2() {
		vec2 v;
		v.x = GetFloat();
		v.y = GetFloat();
		
		return v;
	}


    string GetString() {
        uint l = GetUint();
        string str;
        str.resize(l);
        for (uint i = 0; i < l; i++) {
            str[i] = buffer[index + i];
        }
        index += l;

        return str;
    }


};
