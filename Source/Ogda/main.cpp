//-----------------------------------------------------------------------------
//           Name: main.cpp
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
#include <cstdlib>

#include <Version/version.h>
#include <tclap/CmdLine.h>
#include <XML/Parsers/jobxmlparser.h>
#include <Logging/logdata.h>
#include <Logging/consolehandler.h>
#include <Internal/common.h>
#include <Internal/error.h>
#include <Memory/allocation.h>

#include "ogda_config.h"
#include "jobhandler.h"
#include <cstdlib>
#include <cstdarg>

void SetPercent(const char* message, int percent) {
    if (!config_hide_progress) {
#ifdef PLATFORM_LINUX
        printf("%c[2K", 27);
#endif
        printf("[%d%%]:%s\r", percent, message);
        fflush(stdout);
    }
}

// Replacement for the main program version of the same fucntion
void FatalError(const char* title, const char* fmt, ...) {
    static const int kBufSize = 1024;
    char err_buf[kBufSize];
    va_list args;
    va_start(args, fmt);
    VFormatString(err_buf, kBufSize, fmt, args);
    va_end(args);
    LOGF << title << "," << err_buf << std::endl;
    exit(10);
}

ErrorResponse DisplayError(const char* title, const char* contents, ErrorType type, bool allow_repetition) {
    LOGF << title << ", " << contents << std::endl;
    exit(1);
    return _continue;
}

ErrorResponse DisplayFormatError(ErrorType type,
                                 bool allow_repetition,
                                 const char* title,
                                 const char* fmtcontents,
                                 ...) {
    static const int kBufSize = 2048;
    char err_buf[kBufSize];
    va_list args;
    va_start(args, fmtcontents);
    VFormatString(err_buf, kBufSize, fmtcontents, args);
    va_end(args);
    return DisplayError(title, err_buf, type, allow_repetition);
}

Allocation alloc;

int main(int argc, const char** argv) {
    alloc.Init();

    TCLAP::CmdLine cmd("Ogda", ' ', GetBuildVersion());

    TCLAP::ValueArg<std::string> inputArg("i", "input-dir", "Input Directory", true, "", "string");
    cmd.add(inputArg);
    TCLAP::ValueArg<std::string> outputArg("o", "output-dir", "Output directory", true, "", "string");
    cmd.add(outputArg);
    TCLAP::ValueArg<std::string> jobfileArg("j", "job-file", "job file", true, "", "string");
    cmd.add(jobfileArg);
    TCLAP::ValueArg<std::string> manifestOutArg("", "manifest-output", "Manifest output file, destionation to store a complete manifest of the generated files.", false, "", "string");
    cmd.add(manifestOutArg);
    TCLAP::ValueArg<std::string> manifestInArg("", "manifest-input", "Manifest input file containing a manifest from the previous build, allows the program to skip converstion of some files.", false, "", "string");
    cmd.add(manifestInArg);
    TCLAP::ValueArg<std::string> databaseDirArg("", "database-dir", "Path to database storage for auxiliary storage of shared files.", false, "", "string");
    cmd.add(databaseDirArg);

    TCLAP::SwitchArg debugOutput("d", "debug-output", "Start game with debug output", cmd, false);
    TCLAP::SwitchArg performRemoves("", "perform-removes", "Actually do removes that the program intend, instead of faking them", cmd, false);
    TCLAP::SwitchArg forceRemove("", "force-removes", "Always perform removes, even if there is an error in execution or the destination folder contains files which aren't listed in the manifest.", cmd, false);
    TCLAP::SwitchArg removeUnlisted("", "remove-unlisted", "Remove files in dest folder that aren't mentioned in the manifest. (Requires --force-removes)", cmd, false);

    TCLAP::SwitchArg printMissing("", "print-missing", "List all files which aren't mentioned in the manifest", cmd, false);

    TCLAP::SwitchArg muteMissing("", "mute-missing", "", cmd, false);
    TCLAP::SwitchArg hideProgress("", "hide-progress", "", cmd, false);

    TCLAP::SwitchArg dateModifiedHash("", "date-modified-hash", "Use the date modified as check for file change, less reliable but faster (BROKEN)", cmd, false);

    TCLAP::SwitchArg printDuplicates("", "print-duplicates", "Give warnings about duplicate references and print a line number list of unnecessary references to items that are recursively referenced from elsewhere", cmd, false);

    TCLAP::SwitchArg printItemList("", "print-item-list", "Print all items initially included via the deployment file", cmd, false);

    TCLAP::SwitchArg loadFromDatabase("", "load-from-database", "Load files from a shared auxiliary database", cmd, false);
    TCLAP::SwitchArg saveToDatabase("", "save-to-database", "Save files to a shared auxiliary database", cmd, false);

    TCLAP::ValueArg<int> threadArg("", "threads", "thread count", false, 8, "int");
    cmd.add(threadArg);

    cmd.parse(argc, argv);

    std::string jobfile = jobfileArg.getValue();
    std::string outputfolder = outputArg.getValue();
    std::string inputfolder = inputArg.getValue();
    std::string manifestout = manifestOutArg.getValue();
    std::string manifestin = manifestInArg.getValue();
    std::string databasedir = databaseDirArg.getValue();
    bool debug_output = debugOutput.getValue();
    bool perform_removes = performRemoves.getValue();
    bool force_removes = forceRemove.getValue();
    config_remove_unlisted_files = removeUnlisted.getValue();
    config_print_missing = printMissing.getValue();
    config_mute_missing = muteMissing.getValue();
    config_hide_progress = hideProgress.getValue();
    config_use_date_modified_as_hash = dateModifiedHash.getValue();
    config_print_duplicates = printDuplicates.getValue();
    config_print_item_list = printItemList.getValue();
    config_load_from_database = loadFromDatabase.getValue();
    config_save_to_database = saveToDatabase.getValue();
    int threads = threadArg.getValue();

    int ret = 0;
    ConsoleHandler consoleHandler;
    LogTypeMask level =
        LogSystem::info | LogSystem::warning | LogSystem::error | LogSystem::fatal;

    if (debug_output)
        level |= LogSystem::debug;

    LogSystem::RegisterLogHandler(level, &consoleHandler);

    LOGI << "Starting Ogda " << GetBuildVersion() << std::endl;

    if ((config_load_from_database || config_load_from_database) && databasedir.empty()) {
        LOGW << "Asked to load or/and save database, but missing the --database-dir flag, ignoring these two other flags." << std::endl;
    }

    std::vector<std::string> input_folders;
    input_folders.push_back(inputfolder);
    JobHandler jobhandler(outputfolder, manifestout, manifestin, databasedir, perform_removes, force_removes, threads);

    bool run_was_clean = jobhandler.Run(jobfile);

    ret = run_was_clean ? 0 : 1;

    if (ret != 0)
        LOGE << "One or more error(s) caused unwanted execution, see log for more information" << std::endl;

    LogSystem::DeregisterLogHandler(&consoleHandler);

    alloc.Dispose();

    return ret;
}
