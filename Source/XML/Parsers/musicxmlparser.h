//-----------------------------------------------------------------------------
//           Name: musicxmlparser.h
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

#include <XML/Parsers/xmlparserbase.h>
#include <Utility/strings.h>

#include <map>
#include <vector>
#include <string>
#include <iostream>

class MusicXMLParser : public XMLParserBase {
   public:
    static const uint32_t kErrorNoError = 0UL;
    static const uint32_t kErrorMissingRootElement = 1UL << 1;
    static const uint32_t kErrorDocumentParseError = 1UL << 2;
    static const uint32_t kErrorSongNameTooLong = 1UL << 3;
    static const uint32_t kErrorSongNameMissing = 1UL << 4;
    static const uint32_t kErrorSongTypeTooLong = 1UL << 5;
    static const uint32_t kErrorSongTypeMissing = 1UL << 6;
    static const uint32_t kErrorSongTypeInvalid = 1UL << 7;
    static const uint32_t kErrorSongSegmentMissing = 1UL << 8;
    static const uint32_t kErrorSongFilePathTooLong = 1UL << 9;
    static const uint32_t kErrorSongFilePathMissing = 1UL << 10;
    static const uint32_t kErrorSongStartSegmentTooLong = 1UL << 11;
    static const uint32_t kErrorSongStartSegmentMissing = 1UL << 12;
    static const uint32_t kErrorSongSegmentNameTooLong = 1UL << 13;
    static const uint32_t kErrorSongSegmentNameMissing = 1UL << 14;
    static const uint32_t kErrorSongSegmentPathTooLong = 1UL << 15;
    static const uint32_t kErrorSongSegmentPathMissing = 1UL << 16;
    static const uint32_t kErrorSongMissing = 1UL << 17;
    static const uint32_t kErrorSongSongRefNameTooLong = 1UL << 18;
    static const uint32_t kErrorSongSongRefNameMissing = 1UL << 19;

    // Applies to all types that hold a string path to disk data in the xml
    static const size_t PATH_MAX_LENGTH = 256;
    // Applies too all name attributes, and references to them
    static const size_t NAME_MAX_LENGTH = 32;
    // Applies to the type, controlling how a song is parsed
    static const size_t TYPE_MAX_LENGTH = 16;

    MusicXMLParser();

    class Segment {
       public:
        Segment();

        char name[NAME_MAX_LENGTH];
        char path[PATH_MAX_LENGTH];
    };

    class SongRef {
       public:
        SongRef();

        char name[NAME_MAX_LENGTH];
    };

    class Song {
       public:
        Song();

        char name[NAME_MAX_LENGTH];
        char start_segment[NAME_MAX_LENGTH];
        char type[NAME_MAX_LENGTH];
        char file_path[PATH_MAX_LENGTH];

        std::vector<Segment> segments;
        std::vector<SongRef> songrefs;

        Segment GetStartSegment() const;
    };

    class Music {
       public:
        std::vector<Song> songs;
    };

    Music music;

    uint32_t Load(const std::string &path) override;
    bool Save(const std::string &path) override;
    void Clear() override;
};

inline std::ostream &operator<<(std::ostream &out, const MusicXMLParser::Segment &segment) {
    out << "<Segment name=\"" << segment.name << "\" " << segment.path << "\"/>" << std::endl;

    return out;
}

inline std::ostream &operator<<(std::ostream &out, const MusicXMLParser::Song &song) {
    out << "<Song name = \"" << song.name << "\">" << std::endl;

    out << "<Segments>" << std::endl;

    for (unsigned i = 0; i < song.segments.size(); i++) {
        out << song.segments[i] << std::endl;
    }

    out << "</Segments>" << std::endl;

    out << "<Layers>" << std::endl;

    out << "</Layers>" << std::endl;

    out << "</Song>";

    return out;
}

inline std::ostream &operator<<(std::ostream &out, const MusicXMLParser::Music &music) {
    out << "<Music><Songs>" << std::endl;

    for (unsigned i = 0; i < music.songs.size(); i++) {
        out << music.songs[i] << std::endl;
    }

    out << "</Songs></Music>";

    return out;
}

inline bool operator==(const MusicXMLParser::Song &lhs, const MusicXMLParser::Song &rhs) {
    return strmtch(lhs.name, rhs.name);
}

inline bool operator!=(const MusicXMLParser::Song &lhs, const MusicXMLParser::Song &rhs) {
    return !strmtch(lhs.name, rhs.name);
}

inline bool operator<(const MusicXMLParser::Song &lhs, const MusicXMLParser::Song &rhs) {
    return strcmp(lhs.name, rhs.name);
}

inline bool operator==(const MusicXMLParser::Segment &lhs, const MusicXMLParser::Segment &rhs) {
    return strmtch(lhs.name, rhs.name);
}

inline bool operator!=(const MusicXMLParser::Segment &lhs, const MusicXMLParser::Segment &rhs) {
    return !strmtch(lhs.name, rhs.name);
}

inline bool operator<(const MusicXMLParser::Segment &lhs, const MusicXMLParser::Segment &rhs) {
    return strcmp(lhs.name, rhs.name);
}
