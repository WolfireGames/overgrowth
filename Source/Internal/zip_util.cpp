//-----------------------------------------------------------------------------
//           Name: zip_util.cpp
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
#include "zip_util.h"

#include <Internal/error.h>
#include <Internal/scoped_buffer.h>
#include <Internal/filesystem.h>

#include <Memory/allocation.h>
#include <Logging/logdata.h>

#include <minizip/zip_util_internal.h>
#include <minizip/zip.h>
#include <minizip/unzip.h>
#include <zlib.h>

#if _WIN32
#include <Compat/fileio.h>
#include <windows.h>
#endif

#include <string>
#include <cstring>

const int size_buf = 16384;
const int opt_compress_level = Z_DEFAULT_COMPRESSION;

void ClearZFI(zip_fileinfo& zi) {
    zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour =
        zi.tmz_date.tm_mday = zi.tmz_date.tm_mon = zi.tmz_date.tm_year = 0;
    zi.dosDate = 0;
    zi.internal_fa = 0;
    zi.external_fa = 0;
}

class ZipFile {
    zipFile zf;

   public:
    ZipFile();
    ~ZipFile();
    void Load(const std::string& path, bool append);
    void AddFile(const std::string& src_file_path, const std::string& in_zip_file_path, const std::string& password = "");
};

void ZipFile::Load(const std::string& path, bool append) {
#ifdef USEWIN32IOAPI
    zlib_filefunc64_def ffunc;
    fill_win32_filefunc64A(&ffunc);
    zf = zipOpen2_64(path.c_str(), append ? 2 : 0, NULL, &ffunc);
#else
    zf = zipOpen64(path.c_str(), append ? 2 : 0);
#endif

    if (zf == NULL) {
        FatalError("Error", "Problem opening zipfile %s", path.c_str());
    }
}

void ZipFile::AddFile(const std::string& src_file_path,
                      const std::string& in_zip_file_path,
                      const std::string& password) {
    ScopedBuffer scoped_buf(size_buf);

    zip_fileinfo zi;
    ClearZFI(zi);
    filetime(src_file_path.c_str(), &zi.tmz_date, &zi.dosDate);

    unsigned long crcFile = 0;
    if (!password.empty()) {
        if (getFileCrc(src_file_path.c_str(), scoped_buf.ptr, size_buf, &crcFile) != ZIP_OK) {
            FatalError("Error", "Problem getting CRC of file: %s", src_file_path.c_str());
        }
    }

    // The FSEEKO sometimes casuse segfault on linux debian for no reason, we never have large files anyways.
    int zip64 = false;  // isLargeFile(src_file_path.c_str());

    /* The path name saved, should not include a leading slash.
    if it did, windows/xp and dynazip couldn't read the zip file. */
    const char* savefilenameinzip = in_zip_file_path.c_str();
    while (savefilenameinzip[0] == '\\' || savefilenameinzip[0] == '/') {
        savefilenameinzip++;
    }

    if (zipOpenNewFileInZip3_64(zf, savefilenameinzip, &zi,
                                NULL, 0, NULL, 0, NULL /* comment*/,
                                (opt_compress_level != 0) ? Z_DEFLATED : 0,
                                opt_compress_level, 0,
                                -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                                password.empty() ? NULL : password.c_str(), crcFile, zip64) != ZIP_OK) {
        FatalError("Error", "Could not open %s in zipfile", src_file_path.c_str());
    }

    {
        FILE* file;
        file = FOPEN_FUNC(src_file_path.c_str(), "rb");
        if (file == NULL) {
            FatalError("Error", "Could not open file: %s", src_file_path.c_str());
        }

        int size_read = 0;
        do {
            size_read = (int)fread(scoped_buf.ptr, 1, size_buf, file);
            if (size_read < size_buf && feof(file) == 0) {
                FatalError("Error", "Problem reading file: %s", src_file_path.c_str());
            }
            if (size_read > 0) {
                if (zipWriteInFileInZip(zf, scoped_buf.ptr, size_read) < 0) {
                    FatalError("Error", "Problem writing \"%s\" in the zipfile.", src_file_path.c_str());
                }
            }
        } while (size_read > 0);
        if (file)
            fclose(file);
    }

    if (zipCloseFileInZip(zf) != ZIP_OK) {
        FatalError("Error", "Problem closing %s in zip file.", src_file_path.c_str());
    }
}

ZipFile::ZipFile() : zf(NULL) {}

ZipFile::~ZipFile() {
    if (zf && zipClose(zf, NULL) != ZIP_OK) {
        DisplayError("Error", "Error closing zip file");
    }
}

void Zip(const std::string& src_file_path,
         const std::string& zip_file_path,
         const std::string& in_zip_file_path,
         OverwriteType overwrite) {
    if (overwrite == _NO_OVERWRITE && check_exist_file(zip_file_path.c_str()) != 0) {
        return;
    }

    std::string dir = zip_file_path.substr(0, zip_file_path.find_last_of("\\/") + 1);
    if (CheckWritePermissions(dir.c_str()) != 0) {
        FatalError("Error", "Couldn't write zip-file to %s, make sure you have write permissions", dir.c_str());
        return;
    }

    ZipFile zf;
#ifdef _WIN32
    createfile(zip_file_path.c_str());

    std::wstring w_src_path(GetShortPathNameW(UTF16fromUTF8(src_file_path).c_str(), NULL, NULL), '\0');
    GetShortPathNameW(UTF16fromUTF8(src_file_path).c_str(), &w_src_path[0], w_src_path.size());
    std::string short_src = UTF8fromUTF16(w_src_path);

    std::wstring w_short_zip(GetShortPathNameW(UTF16fromUTF8(zip_file_path).c_str(), NULL, NULL), '\0');
    GetShortPathNameW(UTF16fromUTF8(zip_file_path).c_str(), &w_short_zip[0], w_short_zip.size());
    std::string short_zip = UTF8fromUTF16(w_short_zip);

    zf.Load(short_zip, (overwrite == _APPEND_OVERWRITE));
    zf.AddFile(short_src, in_zip_file_path);
#else
    zf.Load(zip_file_path, (overwrite == _APPEND_OVERWRITE));
    zf.AddFile(src_file_path, in_zip_file_path);
#endif
}

struct ExpandedZipEntry {
    const char* filename;
    const char* data;
    unsigned size;
};

ExpandedZipFile::ExpandedZipFile() : buf(NULL), filename_buf(NULL), entries(NULL) {}

ExpandedZipFile::~ExpandedZipFile() {
    Dispose();
}

void ExpandedZipFile::Dispose() {
    OG_FREE(buf);
    OG_FREE(filename_buf);
    OG_FREE(entries);
}

void ExpandedZipFile::ResizeEntries(unsigned _num_entries) {
    num_entries = _num_entries;
    OG_FREE(entries);
    entries = (ExpandedZipEntry*)OG_MALLOC(sizeof(ExpandedZipEntry) * num_entries);
}

void ExpandedZipFile::ResizeFilenameBuffer(unsigned num_chars) {
    OG_FREE(filename_buf);
    filename_buf = (char*)OG_MALLOC(num_chars);
}

void ExpandedZipFile::ResizeDataBuffer(unsigned num_bytes) {
    OG_FREE(buf);
    buf = (char*)OG_MALLOC(num_bytes);
}

void ExpandedZipFile::SetFilename(unsigned offset, const char* data, unsigned size) {
    memcpy(filename_buf + offset, data, size);
}

void ExpandedZipFile::SetData(unsigned offset, const char* data, unsigned size) {
    memcpy(buf + offset, data, size);
}

void ExpandedZipFile::SetEntry(unsigned which, unsigned file_name_offset, unsigned data_offset, unsigned size) {
    ExpandedZipEntry& entry = entries[which];
    entry.filename = filename_buf + file_name_offset;
    entry.data = buf + data_offset;
    entry.size = size;
}

void ExpandedZipFile::GetEntry(unsigned which, const char*& filename, const char*& data, unsigned& size) {
    ExpandedZipEntry& entry = entries[which];
    filename = entry.filename;
    data = entry.data;
    size = entry.size;
}

class UnZipFile {
    unzFile uf;

   public:
    UnZipFile();
    ~UnZipFile();
    void Load(const std::string& path);
    void Extract(const std::string& in_zip_file_path, const std::string& dst_file_path, OverwriteType overwrite);
    void PrintInfo();
    void ExtractAll(ExpandedZipFile& expanded_zip_file);
};

void UnZipFile::Load(const std::string& path) {
#ifdef USEWIN32IOAPI
    zlib_filefunc64_def ffunc;
    fill_win32_filefunc64A(&ffunc);
    uf = unzOpen2_64(path.c_str(), &ffunc);
#else
    uf = unzOpen64(path.c_str());
#endif

    if (uf == NULL) {
        FatalError("Error", "Problem opening zipfile %s", path.c_str());
    }
}

UnZipFile::UnZipFile() : uf(NULL) {}

UnZipFile::~UnZipFile() {
    if (uf && unzClose(uf) != ZIP_OK) {
        DisplayError("Error", "Error closing unzip file");
    }
}

void do_extract_currentfile(unzFile uf, const std::string& dst, OverwriteType overwrite, const char* password) {
    unz_file_info64 file_info;
    char filename_inzip[256];
    if (unzGetCurrentFileInfo64(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0) != UNZ_OK) {
        FatalError("Error", "Problem getting current unzip file");
    }

    FILE* fout = NULL;

    char* filename_withoutpath = filename_inzip;
    {
        char* p = filename_inzip;
        while ((*p) != '\0') {
            if (((*p) == '/') || ((*p) == '\\'))
                filename_withoutpath = p + 1;
            p++;
        }
    }

    if (unzOpenCurrentFilePassword(uf, password) != UNZ_OK) {
        FatalError("Error", "Error with unzOpenCurrentFilePassword");
    }

    fout = FOPEN_FUNC(dst.c_str(), "wb");
    /* some zipfile don't contain directory alone before file */
    if (fout == NULL) {
        char c = *(filename_withoutpath - 1);
        *(filename_withoutpath - 1) = '\0';
        makedir(dst.c_str());
        *(filename_withoutpath - 1) = c;
        fout = FOPEN_FUNC(dst.c_str(), "wb");
    }

    if (fout == NULL) {
        FatalError("Error", "Error opening %s", dst.c_str());
    }

    if (fout != NULL) {
        LOGI << " extracting: " << dst << std::endl;

        ScopedBuffer scoped_buf(size_buf);
        int err = 0;
        do {
            err = unzReadCurrentFile(uf, scoped_buf.ptr, size_buf);
            if (err < 0) {
                FatalError("Error", "Error reading from UnZip file");
                printf("error %d with zipfile in unzReadCurrentFile\n", err);
                break;
            } else if (err > 0) {
                if (fwrite(scoped_buf.ptr, err, 1, fout) != 1) {
                    FatalError("Error", "Error in writing extracted file");
                }
            }
        } while (err > 0);
        if (fout) {
            fclose(fout);
        }
        change_file_date(dst.c_str(), file_info.dosDate, file_info.tmu_date);
    }

    if (unzCloseCurrentFile(uf) != UNZ_OK) {
        FatalError("Error", "Error with zipfile in unzCloseCurrentFile\n");
    }
}

void UnZipFile::Extract(const std::string& in_zip_file_path, const std::string& dst_file_path, OverwriteType overwrite) {
    if (unzLocateFile(uf, in_zip_file_path.c_str(), 0) != UNZ_OK) {
        FatalError("Error", "File \"%s\" not found in zip file.", in_zip_file_path.c_str());
    }

    do_extract_currentfile(uf, dst_file_path, overwrite, NULL);
}

void UnZipFile::PrintInfo() {
    unz_global_info global_info;
    unzGetGlobalInfo(uf, &global_info);
    LOGI << global_info.number_entry << " files." << std::endl;

    unzGoToFirstFile(uf);

    do {
        unz_file_info file_info;
        char name[256];
        unzGetCurrentFileInfo(uf, &file_info, name, 256, NULL, 0, NULL, 0);
        LOGI << "\t" << name << std::endl;
        LOGI << "\t\tdosDate: " << file_info.dosDate << std::endl;
        LOGI << "\t\tcrc: " << file_info.crc << std::endl;
        LOGI << "\t\tcompressed_size: " << file_info.compressed_size << std::endl;
        LOGI << "\t\tuncompressed_size: " << file_info.uncompressed_size << std::endl;
        tm_unz& date = file_info.tmu_date;
        LOGI << "\t\tDate: " << date.tm_year << ", " << date.tm_mon << "/" << date.tm_mday << ", " << date.tm_hour << ":" << date.tm_min << ":" << date.tm_sec << std::endl;
    } while (unzGoToNextFile(uf) == UNZ_OK);
}

void UnZipFile::ExtractAll(ExpandedZipFile& expanded_zip_file) {
    expanded_zip_file.Dispose();

    {  // Allocate memory for the zip file entries
        unz_global_info global_info;
        unzGetGlobalInfo(uf, &global_info);
        expanded_zip_file.ResizeEntries(global_info.number_entry);
    }

    {  // Get total memory requirements for filenames and data
        unsigned total_size = 0;
        unsigned total_filename_size = 0;

        unzGoToFirstFile(uf);
        do {
            unz_file_info file_info;
            unzGetCurrentFileInfo(uf, &file_info, NULL, 0, NULL, 0, NULL, 0);
            total_size += file_info.uncompressed_size + 1;  // Extra room for '\0'
            total_filename_size += file_info.size_filename + 1;
        } while (unzGoToNextFile(uf) == UNZ_OK);

        expanded_zip_file.ResizeDataBuffer(total_size);
        expanded_zip_file.ResizeFilenameBuffer(total_filename_size);
    }

    {  // Load filenames and decompress data
        unsigned entry_id = 0;
        unsigned data_offset = 0;
        unsigned filename_offset = 0;
        ScopedBuffer scoped_buf(size_buf);
        unzGoToFirstFile(uf);
        do {
            char filename_buf[256];
            unz_file_info file_info;
            unzGetCurrentFileInfo(uf, &file_info, filename_buf, 256, NULL, 0, NULL, 0);
            if (file_info.size_filename > 256) {
                FatalError("Error", "Zip file contains filename with length greater than 256");
            }
            expanded_zip_file.SetEntry(entry_id, filename_offset, data_offset, file_info.uncompressed_size);
            ++entry_id;
            expanded_zip_file.SetFilename(filename_offset, filename_buf, file_info.size_filename + 1);
            filename_offset += file_info.size_filename + 1;

            unzOpenCurrentFile(uf);
            int bytes_read = 0;
            do {
                bytes_read = unzReadCurrentFile(uf, scoped_buf.ptr, size_buf);
                if (bytes_read < 0) {
                    FatalError("Error", "Error reading from UnZip file");
                } else if (bytes_read > 0) {
                    expanded_zip_file.SetData(data_offset, (char*)scoped_buf.ptr, bytes_read);
                    data_offset += bytes_read;
                }
            } while (bytes_read > 0);
            char zero = '\0';
            expanded_zip_file.SetData(data_offset, &zero, 1);
            ++data_offset;
            unzCloseCurrentFile(uf);
        } while (unzGoToNextFile(uf) == UNZ_OK);
    }
}

void UnZip(const std::string& zip_file_path,
           ExpandedZipFile& expanded_zip_file) {
    UnZipFile uf;
#ifdef _WIN32
    // File exists, so just get the short path
    std::wstring short_zip_file_path(GetShortPathNameW(UTF16fromUTF8(zip_file_path).c_str(), NULL, NULL), '\0');
    GetShortPathNameW(UTF16fromUTF8(zip_file_path).c_str(), &short_zip_file_path[0], short_zip_file_path.size());
    uf.Load(UTF8fromUTF16(short_zip_file_path));
#else
    uf.Load(zip_file_path);
#endif
    uf.ExtractAll(expanded_zip_file);
}

void UnZipToFile(const std::string& dst_file_path,
                 const std::string& zip_file_path,
                 const std::string& in_zip_file_path,
                 OverwriteType overwrite) {
    if (overwrite == _NO_OVERWRITE && check_exist_file(zip_file_path.c_str()) != 0) {
        return;
    }

    UnZipFile uf;
    uf.Load(zip_file_path);
    uf.Extract(in_zip_file_path, dst_file_path, overwrite);
}

void PrintZipFileInfo(const std::string& zip_file_path) {
    UnZipFile uf;
    uf.Load(zip_file_path);
    LOGI << "Zip file \"" << zip_file_path << "\" contains:" << std::endl;
    uf.PrintInfo();
}
