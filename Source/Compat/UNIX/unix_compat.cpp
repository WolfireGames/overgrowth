//-----------------------------------------------------------------------------
//           Name: unix_compat.cpp
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
#include <Compat/platform.h>
#if !PLATFORM_UNIX
#error Do not compile this.
#endif
#include "unix_compat.h"

#include <Compat/compat.h>
#include <Compat/fileio.h>

#include <Internal/common.h>
#include <Internal/filesystem.h>

#include <Memory/allocation.h>
#include <Logging/logdata.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#if 1
#define dprintf printf
#else
static inline void dprintf(const char *fmt, ...) {}
#endif

void WorkingDir(string *dir) {
    char cwd[kPathSize];
    char *ret = getcwd(cwd, kPathSize);
    if (ret != NULL) {
        *dir = cwd;
    }
}

std::string GetAbsPath(const char *rel) {
    char path[PATH_MAX];

    if (realpath(rel, path) != NULL) {
        return std::string(path);
    } else {
        return std::string();
    }
}

int initWriteDir(char *path, int buf_size) {
    std::string write_dir;

#if PLATFORM_MACOSX
    const char *home_env = getenv("HOME");
    if (home_env == NULL) {
        home_env = "./";
    }
    write_dir += home_env;
    write_dir += "/Library/Application Support/Overgrowth/";
#else
    const char *env_path = getenv("XDG_DATA_HOME");
    if (env_path) {
        write_dir += env_path;
    } else {
        env_path = getenv("HOME");
        if (!env_path) {
            env_path = "./";
        }
        write_dir += env_path;
        write_dir += "/.local/share";
    }
    write_dir += "/Overgrowth/";
#endif

    FormatString(path, buf_size, "%s", write_dir.c_str());

    return 0;
}

static char *findBinaryInPath(const char *bin, char *envr) {
    size_t alloc_size = 0;
    char *exe = NULL;
    char *start = envr;
    char *ptr;

    do {
        size_t size;
        ptr = strchr(start, ':');  // find next $PATH separator.
        if (ptr)
            *ptr = '\0';

        size = strlen(start) + strlen(bin) + 2;
        if (size > alloc_size) {
            char *x = (char *)realloc(exe, size);
            if (x == NULL) {
                if (exe != NULL)
                    OG_FREE(exe);
                return NULL;
            }

            alloc_size = size;
            exe = x;
        }

        // build full binary path...
        strcpy(exe, start);
        if ((exe[0] == '\0') || (exe[strlen(exe) - 1] != '/'))
            strcat(exe, "/");
        strcat(exe, bin);

        if (access(exe, X_OK) == 0)  // Exists as executable? We're done.
        {
            strcpy(exe, start);  // i'm lazy. piss off.
            return (exe);
        } /* if */

        start = ptr + 1;  // start points to beginning of next element.
    } while (ptr != NULL);

    if (exe != NULL)
        OG_FREE(exe);

    return (NULL);  // doesn't exist in path.
}

void chdirToBasePath(const char *argv0) {
    char *newdir = NULL;
    const char *find = strchr(argv0, '/');
    const char *envr = getenv("PATH");

    if ((find == NULL) && (envr == NULL))
        return;

    if (find != NULL) {  // Has a real path
        newdir = (char *)OG_MALLOC(strlen(argv0) + 1);
        strcpy(newdir, argv0);
    } else {
        // If there isn't a path on argv0, then look through the $PATH for it.
        char *envrcpy = new char[strlen(envr) + 1];
        strcpy(envrcpy, envr);
        newdir = findBinaryInPath(argv0, envrcpy);
        delete[] envrcpy;
    }

    if (newdir) {
        char *ptr = strrchr(newdir, '/');
        if (ptr)
            *ptr = '\0';

        char realbuf[PATH_MAX];
        if (realpath(newdir, realbuf) == NULL)
            strcpy(realbuf, newdir);

        OG_FREE(newdir);

        // dprintf("basepath '%s'\n", realbuf);
        chdir(realbuf);
    }
}

static void caseCorrectFilename(char *path) {
    if (*path == '\0')
        return;  // this is the root directory ("/"), so just return.

    if (access(path, F_OK) == 0)
        return;  // fast path: it definitely exists in acceptable case.

    DIR *dir = NULL;
    char *base = strrchr(path, '/');
    if (base == path)
        dir = opendir("/");
    else if (base == NULL) {
        dir = opendir(".");
        base = path - 1;
    } else {
        *base = '\0';
        dir = opendir(path);
        *base = '/';
    }
    base++;

    if (dir == NULL)
        return;  // basedir doesn't exist at all, handled elsewhere.

    if (*base != '\0') {
        struct dirent *dent;
        while ((dent = readdir(dir)) != NULL) {
            if (strcasecmp(base, dent->d_name) == 0)  // found a match.
            {
                strcpy(base, dent->d_name);  // make sure case is right.
                break;
            }
        }
    }

    closedir(dir);
}

std::string caseCorrect(const std::string &path) {
    char *buf = (char *)OG_MALLOC((path.length() + 1) * sizeof(char));

    strncpy(buf, path.c_str(), path.length() + 1);

    caseCorrect(buf);

    std::string ret(buf);
    OG_FREE(buf);
    return ret;
}

// TODO: figure out long-term solution for case-sensitivity
void caseCorrect(char *path) {
    for (int i = 0; path[i]; i++) {
        if ((path[i] == '/') || (path[i] == '\\')) {
            path[i] = '\0';
            caseCorrectFilename(path);
            path[i] = '/';
        }
    }

    // get last piece of the path.
    caseCorrectFilename(path);
}

std::vector<std::string> &getSubdirectories(const char *basepath, std::vector<std::string> &mods) {
    DIR *moddir = opendir(basepath);
    std::string path(basepath);

    if (moddir) {
        struct dirent *entry;

        while ((entry = readdir(moddir))) {
            if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0) {
                std::string fullPath;
                if (path[path.length() - 1] == '/') {
                    fullPath = path + std::string(entry->d_name);
                } else {
                    fullPath = path + "/" + std::string(entry->d_name);
                }

                struct stat filestat;

                lstat(fullPath.c_str(), &filestat);

                if (S_ISDIR(filestat.st_mode) || S_ISLNK(filestat.st_mode)) {
                    mods.push_back(fullPath);
                }
            }
        }
        closedir(moddir);
    } else {
    }

    return mods;
}

std::vector<std::string> &getDeepManifest(const char *basepath, const char *prefix, std::vector<std::string> &files) {
    std::string s_prefix(prefix);
    DIR *moddir = opendir(basepath);
    std::string path(basepath);

    if (moddir) {
        struct dirent entrydata;
        struct dirent *entry;

        while (!readdir_r(moddir, &entrydata, &entry) && entry) {
            if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0) {
                std::string fullPath;
                std::string sub_prefix;
                if (path[path.length() - 1] == '/') {
                    fullPath = path + std::string(entry->d_name);
                    sub_prefix = s_prefix + std::string(entry->d_name);
                } else {
                    fullPath = path + "/" + std::string(entry->d_name);

                    if (s_prefix.length() == 0) {
                        sub_prefix = std::string(entry->d_name);
                    } else {
                        sub_prefix = s_prefix + "/" + std::string(entry->d_name);
                    }
                }

                struct stat filestat;
                // Using lstat because the file mode in dirent doesnt exist outside linux and bsd.
                lstat(fullPath.c_str(), &filestat);

                if (S_ISDIR(filestat.st_mode) || S_ISLNK(filestat.st_mode)) {
                    getDeepManifest(fullPath.c_str(), sub_prefix.c_str(), files);
                } else if (S_ISREG(filestat.st_mode)) {
                    files.push_back(sub_prefix);
                }
            }
        }
        closedir(moddir);
    } else {
    }

    return files;
}

bool fileExist(const char *path) {
    return access(path, F_OK) == 0;
}

bool fileReadable(const char *path) {
    return access(path, R_OK) == 0;
}

bool isFile(const char *path) {
    struct stat filestat;
    // Using lstat because the file mode in dirent doesnt exist outside linux and bsd.
    if (lstat(path, &filestat) == 0) {
        if (S_ISREG(filestat.st_mode)) {
            return true;
        }
    } else {
        LOGE << "Could not lstat " << path << std::endl;
        return false;
    }

    return false;
}

bool areSame(const char *path1, const char *path2) {
    struct stat statbuf1;
    struct stat statbuf2;

    int status1 = stat(path1, &statbuf1);
    int status2 = stat(path2, &statbuf2);

    if (status1 == 0 && status2 == 0) {
        // Check if same inode on same device
        return (statbuf1.st_ino == statbuf2.st_ino && statbuf1.st_dev == statbuf2.st_dev);
    } else {
        LOGE << "Unable to stat " << path1 << " and/or " << path2 << std::endl;
        return false;
    }
}

std::string dumpIntoFile(const void *buf, size_t nbyte) {
    char path[512];
    snprintf(path, 256, "%s/OGDAFILE.XXXXXX", P_tmpdir);

    int fd = mkstemp(path);

    if (fd == -1) {
        LOGE << "Error creating a tmp file" << std::endl;
        exit(1);
    } else {
        if ((ssize_t)nbyte != write(fd, buf, nbyte)) {
            LOGE << "Didn't write as much as expected in dumpIntoFile" << std::endl;
        }

        close(fd);
    }

    return std::string(path);
}

bool checkFileAccess(const char *path) {
    return access(path, R_OK) != -1;
}

void createParentDirs(const char *abs_path) {
    char build_path[kPathSize] = {'\0'};
    for (int i = 0; abs_path[i] != '\0'; ++i) {
        if (abs_path[i] == '/') {
            if (!CheckFileAccess(build_path)) {
                mkdir(build_path, S_IRWXU);
            }
        }
        build_path[i] = abs_path[i];
    }
}

int os_movefile(const char *source, const char *dest) {
    return rename(source, dest);
}

int os_deletefile(const char *path) {
    return remove(path);
}

int os_createfile(const char *path) {
    FILE *file = fopen(path, "w");
    if (file) {
        fclose(file);
        return 0;
    } else {
        return 1;
    }
}

int os_fileexists(const char *path) {
    return access(path, F_OK);
}

// end of unix_compat.cpp ...
