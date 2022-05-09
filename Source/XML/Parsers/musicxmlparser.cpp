//-----------------------------------------------------------------------------
//           Name: musicxmlparser.cpp
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
#include "musicxmlparser.h"

#include <Utility/commonregex.h>
#include <Utility/strings.h>

#include <XML/xml_helper.h>
#include <Logging/logdata.h>

#include <tinyxml.h>

#include <algorithm>

MusicXMLParser::Segment::Segment() {
    name[0] = '\0';
    path[0] = '\0';
}

MusicXMLParser::SongRef::SongRef() {
    name[0] = '\0';
}

MusicXMLParser::Song::Song() {
    name[0] = '\0';
    start_segment[0] = '\0';
    type[0] = '\0';
    file_path[0] = '\0';
}

MusicXMLParser::MusicXMLParser() {
}

uint32_t MusicXMLParser::Load(const std::string& path) {
    Clear();

    TiXmlDocument doc(path.c_str());
    doc.LoadFile();

    uint32_t error = kErrorNoError;
    ;
    int err;

    if (!doc.Error()) {
        TiXmlElement* pRoot = doc.RootElement();

        if (pRoot) {
            TiXmlHandle hRoot(pRoot);

            TiXmlElement* eSong = hRoot.FirstChild("Song").Element();

            while (eSong) {
                Song song;

                err = strscpy(song.name, eSong->Attribute("name"), NAME_MAX_LENGTH);
                if (err == SOURCE_TOO_LONG) {
                    error |= kErrorSongNameTooLong;
                } else if (err == SOURCE_IS_NULL) {
                    error |= kErrorSongNameMissing;
                }

                err = strscpy(song.type, eSong->Attribute("type"), NAME_MAX_LENGTH);
                if (err == SOURCE_TOO_LONG) {
                    error |= kErrorSongTypeTooLong;
                } else if (err == SOURCE_IS_NULL) {
                    error |= kErrorSongTypeMissing;
                }

                if (strmtch(song.type, "single")) {
                    err = strscpy(song.file_path, eSong->Attribute("file_path"), PATH_MAX_LENGTH);
                    if (err == SOURCE_TOO_LONG) {
                        error |= kErrorSongFilePathTooLong;
                    } else if (err == SOURCE_IS_NULL) {
                        error |= kErrorSongFilePathMissing;
                    }
                } else if (strmtch(song.type, "segmented")) {
                    err = strscpy(song.start_segment, eSong->Attribute("start_segment"), NAME_MAX_LENGTH);
                    if (err == SOURCE_TOO_LONG) {
                        error |= kErrorSongStartSegmentTooLong;
                    } else if (err == SOURCE_IS_NULL) {
                        error |= kErrorSongStartSegmentMissing;
                    }

                    TiXmlElement* eSegment = eSong->FirstChildElement("Segment");

                    while (eSegment) {
                        Segment segment;

                        err = strscpy(segment.name, eSegment->Attribute("name"), NAME_MAX_LENGTH);
                        if (err == SOURCE_TOO_LONG) {
                            error |= kErrorSongSegmentNameTooLong;
                        } else if (err == SOURCE_IS_NULL) {
                            error |= kErrorSongStartSegmentMissing;
                        }

                        err = strscpy(segment.path, eSegment->Attribute("path"), PATH_MAX_LENGTH);
                        if (err == SOURCE_TOO_LONG) {
                            error |= kErrorSongSegmentPathTooLong;
                        } else if (err == SOURCE_IS_NULL) {
                            error |= kErrorSongSegmentPathMissing;
                        }

                        if (std::find(song.segments.begin(), song.segments.end(), segment) == song.segments.end()) {
                            song.segments.push_back(segment);
                        } else {
                            LOGE << "Segment " << segment << " already has a copy in the song, skipping." << std::endl;
                        }

                        eSegment = eSegment->NextSiblingElement("Segment");
                    }

                    if (song.segments.size() == 0) {
                        error |= kErrorSongSegmentMissing;
                    }

                } else if (strmtch(song.type, "layered")) {
                    TiXmlElement* eSongRef = eSong->FirstChildElement("SongRef");
                    while (eSongRef) {
                        SongRef sr;
                        err = strscpy(sr.name, eSongRef->Attribute("name"), NAME_MAX_LENGTH);
                        if (err == SOURCE_TOO_LONG) {
                            error |= kErrorSongSongRefNameTooLong;
                        } else if (err == SOURCE_IS_NULL) {
                            error |= kErrorSongSongRefNameMissing;
                        }
                        song.songrefs.push_back(sr);

                        eSongRef = eSongRef->NextSiblingElement("SongRef");
                    }
                } else {
                    error |= kErrorSongTypeInvalid;
                }

                if (std::find(music.songs.begin(), music.songs.end(), song) == music.songs.end()) {
                    music.songs.push_back(song);
                } else {
                    LOGE << "Song " << song << " already has matching instance in the song list structure, skipping." << std::endl;
                }

                eSong = eSong->NextSiblingElement("Song");
            }

            if (music.songs.size() == 0) {
                error |= kErrorSongMissing;
            }
        } else {
            error |= kErrorSongMissing;
        }
    } else {
        error |= kErrorDocumentParseError;
        LOGE << "Unable to parse xml document:" << path << " " << doc.ErrorDesc() << std::endl;
    }

    return error;
}

bool MusicXMLParser::Save(const std::string& path) {
    LOGE << "No saving routine implemented for MusixXMLParser" << std::endl;
    return false;
}

void MusicXMLParser::Clear() {
    music = Music();
}

MusicXMLParser::Segment MusicXMLParser::Song::GetStartSegment() const {
    std::vector<MusicXMLParser::Segment>::const_iterator segit;

    for (segit = segments.begin(); segit != segments.end(); segit++) {
        if (strmtch(segit->name, start_segment)) {
            return *segit;
        }
    }
    LOGE << "Unable to find start segment with name " << start_segment << std::endl;
    return MusicXMLParser::Segment();
}
