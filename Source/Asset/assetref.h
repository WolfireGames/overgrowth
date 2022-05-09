//-----------------------------------------------------------------------------
//           Name: assetref.h
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
#pragma once

#include <Asset/assetbase.h>
#include <Asset/assettypes.h>

class AssetRefBase {
   protected:
    AssetRefBase();

   public:
    virtual ~AssetRefBase();
    virtual AssetType GetType() = 0;
};

template <class T>
class AssetRef : public AssetRefBase {
   private:
    T* asset_ptr_;

   public:
    AssetType GetType() override {
        return T::GetType();
    };

    void clear() {
        if (asset_ptr_ != NULL) {
            asset_ptr_->DecrementRefCount();
            asset_ptr_ = NULL;
        }
    }

    bool valid() const {
        return asset_ptr_ != NULL;
    }

    AssetRef() {
        asset_ptr_ = NULL;
    }

    AssetRef(T* _asset_ptr) {
        asset_ptr_ = _asset_ptr;
        if (asset_ptr_ != NULL) {
            asset_ptr_->IncrementRefCount();
        }
    }

    AssetRef(const AssetRef& other) {
        asset_ptr_ = other.asset_ptr_;
        if (asset_ptr_ != NULL) {
            asset_ptr_->IncrementRefCount();
        }
    }

    ~AssetRef() override {
        if (asset_ptr_ != NULL) {
            asset_ptr_->DecrementRefCount();
            asset_ptr_ = NULL;
        }
    }

    T& operator*() {
        return *asset_ptr_;
    }

    const T& operator*() const {
        return *asset_ptr_;
    }

    const AssetRef& operator=(const AssetRef& other) {
        clear();
        asset_ptr_ = other.asset_ptr_;
        if (asset_ptr_) {
            asset_ptr_->IncrementRefCount();
        }
        return *this;
    }

    bool operator!=(const AssetRef& other) const {
        return asset_ptr_ != other.asset_ptr_;
    }

    bool operator==(const AssetRef& other) const {
        return asset_ptr_ == other.asset_ptr_;
    }

    bool operator<(const AssetRef& other) const {
        return asset_ptr_ < other.asset_ptr_;
    }

    T* operator->() const {
        return asset_ptr_;
    }
};
