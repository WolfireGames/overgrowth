//-----------------------------------------------------------------------------
//           Name: config.h
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

#include <Graphics/textures.h>
#include <Graphics/camera.h>

#include <Internal/datemodified.h>
#include <Math/vec2math.h>

#include <SDL_keycode.h>

#include <map>
#include <string>
#include <sstream>
#include <iostream>

/**
 * The StringVariant class is a simple string-based variant implementation that allows
 * the user to easily convert between simple numeric/string types.
 */
class StringVariant {
    std::string data;

   public:
    StringVariant() : data() {}

    template <typename ValueType>
    StringVariant(ValueType val) {
        std::ostringstream stream;
        stream << val;
        data = stream.str();
    }

    template <typename ValueType>
    StringVariant& operator=(const ValueType val) {
        std::ostringstream stream;
        stream << val;
        data.assign(stream.str());

        return *this;
    }

    template <typename NumberType>
    NumberType toNumber() const {
        NumberType result = 0;
        std::istringstream stream(data);
        if (stream >> result) {
            return result;
        } else if (data == "yes" || data == "true") {
            return 1;
        } else {
            return 0;
        }
    }

    bool toBool() const {
        return toNumber<int>() == 1;
    }

    std::string str() const {
        return data;
    }

    bool operator==(const StringVariant& other) const;
    bool operator!=(const StringVariant& other) const;
};

struct ConfigVal {
    StringVariant data;
    int order;
    bool operator==(const ConfigVal& other) const;
};

struct Resolution {
    Resolution(int w, int h);
    int w, h;
};

class ResolutionCompare {
   public:
    bool operator()(const Resolution& a, const Resolution& b) {
        return (a.w * a.h) > (b.w * b.h);
    }
};

/**
 * The Config class can be used to load simple key/value pairs from a file.
 *
 * @note An example of syntax:
 *    // An example of a comment
 *    username: Bob
 *    gender: male
 *    hair-color: black // inline comments are also allowed
 *    level: 42
 *
 * @note An example of usage:
 *    Config config;
 *    config.load("myFile.txt");
 *
 *    std::string username = config["username"].str();
 *    int level = config["level"].toNumber<int>();
 */

class Config {
   public:
    // Keep track of path and date modified so we can live-update
    typedef std::map<std::string, ConfigVal> Map;
    Map map_;         // The actual map used to store key/value pairs
    Map shadow_map_;  // Runtime hidden values.

    // Boolean keeping track on if we believe this config has changed.
    bool has_changed_since_save;

    Config();

    // Loads key/value pairs from a file
    // Returns whether or not this operation was successful
    bool Load(const std::string& filename, bool just_filling_blanks = false, bool shadow_variables = false);
    bool Load(std::istream& data, bool just_filling_blanks = false, bool shadow_variables = false);
    bool Save(const std::string& filename);
    int GetMonitorCount();
    std::vector<Resolution> GetPossibleResolutions();
    void SetSettingsToPreset(std::string preset_name);
    std::string GetSettingsPreset();
    std::vector<std::string> GetSettingsPresets();
    std::vector<std::string> GetDifficultyPresets();
    std::string GetDifficultyPreset();
    std::string GetClosestDifficulty();
    void SetDifficultyPreset(std::string name);

    void ReloadStaticSettings();
    void ReloadDynamicSettings();

    // Use the [] operator to access values just like a map container
    // If a key does not exist, will return an empty StringVariant
    const StringVariant& operator[](const std::string& keyName) const;

    bool HasKey(const char* key);
    bool HasKey(std::string& key);

    void RemoveConfig(std::string index);

    bool HasChangedSinceLastSave();

    template <typename T>
    StringVariant& GetRef(T key) {
        has_changed_since_save = true;
        return map_[key].data;
    }

    // Use the [] operator to get/set values just like a map container
    // StringVariant& operator[](const std::string& keyName){ return map_[keyName].data; }

    bool operator==(const Config& other) const;
    bool operator!=(const Config& other) const;

    static Config* GetPresets();

    bool PrimarySourceModified();
    inline std::string GetPrimaryPath() { return primary_path_; }

    std::vector<Resolution>& GetCommonResolutions();

   private:
    std::string primary_path_;
    int64_t date_modified_;

    std::vector<Resolution> commonResolutions;
};

extern Config config;
