//-----------------------------------------------------------------------------
//           Name: referencecounter.h
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
#pragma once
#ifndef NULL
#define NULL 0
#endif

template <typename T>
class ReferenceCounter {
   private:
    int* ref_counter;
    T* data;

   public:
    ReferenceCounter() : ref_counter(new int(1)),
                         data(new T()) {
    }

    /*
    ReferenceCounter(const T& _data):
    ref_counter(new int(1)),
    data(new T(_data))
    {
    }
    */

    ReferenceCounter(T* _data) : ref_counter(new int(1)),
                                 data(_data) {
    }

    void Set(T* _data) {
        *ref_counter -= 1;
        if (*ref_counter == 0) {
            delete ref_counter;
            if (data) {
                delete data;
            }
            data = NULL;
            ref_counter = NULL;
        }

        data = _data;
        ref_counter = new int(1);
    }

    ReferenceCounter(const ReferenceCounter<T>& other) {
        data = other.data;
        ref_counter = other.ref_counter;

        *ref_counter += 1;
    }

    ReferenceCounter& operator=(const ReferenceCounter<T>& other) {
        if (&other == this) {
            return *this;
        }

        *ref_counter -= 1;
        if (*ref_counter == 0) {
            delete ref_counter;
            if (data) {
                delete data;
            }
            data = NULL;
            ref_counter = NULL;
        }

        data = other.data;
        ref_counter = other.ref_counter;

        *ref_counter += 1;

        return *this;
    }

    T* operator->() {
        return data;
    }

    ~ReferenceCounter() {
        *ref_counter -= 1;
        if (*ref_counter == 0) {
            delete ref_counter;
            delete data;
            data = NULL;
            ref_counter = NULL;
        }
    }

    const T& GetConst() const {
        return *data;
    }

    T* GetPtr() {
        return data;
    }

    template <typename H>
    H* GetPtr() {
        return static_cast<H*>(data);
    }

    template <typename H>
    const H* GetConstPtr() const {
        return static_cast<H*>(data);
    }

    const bool Valid() const {
        return data != NULL;
    }

    int GetReferenceCount() const {
        return ref_counter ? *ref_counter : 0;
    }
};
