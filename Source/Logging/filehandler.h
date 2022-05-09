//-----------------------------------------------------------------------------
//           Name: filehandler.h
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

#include <Compat/fileio.h>
#include <Logging/loghandler.h>

/*!
        Potential child of LogHandler and will print messages to file.
*/
class FileHandler : public LogHandler {
   public:
    /*!
    \param Type is the channel to use this object.
    \param Path is the relative path to the file.
    \param Append will cause the messages to be appended to the file.
    */
    FileHandler(std::string path, size_t _startup_file_size, size_t _max_file_size);

    ~FileHandler() override;

    /*!
            \param message will print message to file if file is open
    */
    void Log(LogSystem::LogType type, int row, const char* filename, const char* cat, const char* message_prefix, const char* message) override;

    void Flush() override;

    void SetMaxWriteLimit(size_t size);

   private:
    std::fstream m_file;
    size_t startup_file_size;
    size_t max_file_size;
};
