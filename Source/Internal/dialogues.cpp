//-----------------------------------------------------------------------------
//           Name: dialogues.cpp
//      Developer: Wolfire Games LLC
//    Description: This is a simple wrapper for displaying save/load dialogue boxes
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
#include "dialogues.h"

#include <Internal/filesystem.h>
#include <Internal/snprintf.h>

#include <Logging/logdata.h>
#include <UserInput/input.h>
#include <Compat/compat.h>
#include <Wrappers/linux_gtk.h>

#ifdef __APPLE__
#include <Compat/Mac/os_file_dialogs_mac.h>
#endif

#include <SDL.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <direct.h>
#include <CommDlg.h>
#include <strsafe.h>
#endif

using namespace Dialog;

#ifdef _WIN32
static DialogErr SetUpWindowsFilterString(wchar_t* buffer, int buffer_size, int BUFFER_SIZE, const wchar_t* extension) {
    size_t cur_len = 0;

    // Build description string, e.g.: "(*.tga; *.jpg; *.dds)\0*.tga;*.jpg;*.dds"
    if (StringCbPrintfW(&buffer[cur_len], BUFFER_SIZE - (cur_len + 1), L"(") != S_OK)
        return INTERNAL_BUFFER_TOO_SMALL;
    cur_len += wcslen(&buffer[cur_len]);

    for (size_t offset = 0; offset < buffer_size;) {
        if (offset > 0) {
            if (StringCbPrintfW(&buffer[cur_len], BUFFER_SIZE - (cur_len + 1), L"; ") != S_OK)
                return INTERNAL_BUFFER_TOO_SMALL;
            cur_len += wcslen(&buffer[cur_len]);
        }

        if (StringCbPrintfW(&buffer[cur_len], BUFFER_SIZE - (cur_len + 1), L"*.%s", &extension[offset]) != S_OK)
            return INTERNAL_BUFFER_TOO_SMALL;
        cur_len += wcslen(&buffer[cur_len]);

        offset += wcslen(&extension[offset]) + 1;
    }

    if (StringCbPrintfW(&buffer[cur_len], BUFFER_SIZE - (cur_len + 1), L")") != S_OK)
        return INTERNAL_BUFFER_TOO_SMALL;
    cur_len += wcslen(&buffer[cur_len]) + 1;

    for (size_t offset = 0; offset < buffer_size;) {
        if (offset > 0) {
            if (StringCbPrintfW(&buffer[cur_len], BUFFER_SIZE - (cur_len + 1), L";") != S_OK)
                return INTERNAL_BUFFER_TOO_SMALL;
            cur_len += wcslen(&buffer[cur_len]);
        }

        if (StringCbPrintfW(&buffer[cur_len], BUFFER_SIZE - (cur_len + 1), L"*.%s", &extension[offset]) != S_OK)
            return INTERNAL_BUFFER_TOO_SMALL;
        cur_len += wcslen(&buffer[cur_len]);

        offset += wcslen(&extension[offset]) + 1;
    }

    // Separate filter from wildcard option with null
    cur_len += 1;

    // Add wildcard option description: "All Files (*.*)\0*.*"
    if (StringCbPrintfW(&buffer[cur_len], BUFFER_SIZE - (cur_len + 1), L"All Files (*.*)") != S_OK)
        return INTERNAL_BUFFER_TOO_SMALL;
    cur_len += wcslen(&buffer[cur_len]) + 1;

    if (StringCbPrintfW(&buffer[cur_len], BUFFER_SIZE - (cur_len + 1), L"*.*") != S_OK)
        return INTERNAL_BUFFER_TOO_SMALL;
    cur_len += wcslen(&buffer[cur_len]) + 1;

    // Add final double-null to end the list
    buffer[cur_len] = '\0';

    return NO_ERR;
}

static DialogErr FormatFullPath(wchar_t* buffer, int BUFFER_SIZE, const wchar_t* initial_dir) {
    if (!_wgetcwd(buffer, BUFFER_SIZE)) {
        return GET_CWD_FAILED;
    }
    if ((int)(wcslen(buffer) + 1 + wcslen(initial_dir)) >= BUFFER_SIZE) {
        return INTERNAL_BUFFER_TOO_SMALL;
    }
    wcscat(buffer, L"\\");
    wcscat(buffer, initial_dir);
    // Convert / to \ for Windows
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (buffer[i] == '\0') {
            break;
        } else if (buffer[i] == '/') {
            buffer[i] = '\\';
        }
    }
    return NO_ERR;
}
#endif

static bool has_valid_dialogues = false;
void Dialog::Initialize() {
#if PLATFORM_UNIX
#ifndef __APPLE__
    has_valid_dialogues = gtk_init_check(NULL, NULL);
#else
    has_valid_dialogues = true;
#endif
#else
    has_valid_dialogues = true;
#endif
}

DialogErr Dialog::readFile(const char* extension, int extension_count, const char* initial_dir, char* path_buffer, int PATH_BUFFER_SIZE) {
    if (has_valid_dialogues == false) {
        LOGE << "Dialogue system was not initialized" << std::endl;
        return UNKNOWN_ERR;
    }

    char initial_dir_abs_path[kPathSize];
    FindFilePath(initial_dir, initial_dir_abs_path, kPathSize, kDataPaths);
#if PLATFORM_UNIX
#ifndef __APPLE__

    GtkWidget* dialog;
    DialogErr err;
    dialog = gtk_file_chooser_dialog_new("Open File",
                                         NULL,
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (initial_dir) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), initial_dir);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        int target_size = snprintf(path_buffer, PATH_BUFFER_SIZE, "%s", filename);
        if ((int)strlen(path_buffer) < target_size) {
            err = USER_BUFFER_TOO_SMALL;
        }
        err = NO_ERR;  // TODO: This clobbers the previous error. Should it be removed? Should it be placed before the if?

        g_free(filename);
    } else {
        err = NO_SELECTION;
    }

    gtk_widget_hide(dialog);
    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) gtk_main_iteration();
    return err;
#else
    UIShowCursor(true);

    char* filename = NULL;
    DialogErr err = OsFileDialogsMac::OpenDialog(extension, initial_dir, &filename);

    if (err == NO_ERR) {
        if (filename != NULL) {
            int target_size = snprintf(path_buffer, PATH_BUFFER_SIZE, "%s", filename);

            if ((int)strlen(path_buffer) < target_size) {
                err = USER_BUFFER_TOO_SMALL;
            }

            free(filename);
        } else {
            err = NO_SELECTION;  // TODO: Is this the right error?
        }
    }

    UIShowCursor(false);

    return err;
#endif
#else
    // Get the initial home directory
    wchar_t initial_working_directory[kPathSize];
    if (!_wgetcwd(initial_working_directory, kPathSize)) {
        return GET_CWD_FAILED;
    }

    wchar_t wide_extension[kPathSize];
    size_t length = 0;
    for (int i = 0; i < extension_count; ++i) {
        length += strlen(&extension[length]) + 1;
    }

    MultiByteToWideChar(CP_UTF8, 0, extension, (int)length, wide_extension, kPathSize);

    // Create string specifying file types that can be opened
    wchar_t buffer[kPathSize];
    DialogErr err = SetUpWindowsFilterString(buffer, (int)length, kPathSize, wide_extension);
    if (err != NO_ERR) {
        return err;
    }

    // Set up file name structure
    OPENFILENAMEW ofn = {0};
    wchar_t szFileName[MAX_PATH] = L"";
    ofn.lpstrFile = szFileName;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = wide_extension;

    // Convert / to \ in path; Windows is picky about that
    for (char* c = initial_dir_abs_path; *c != '\0'; ++c) {
        if (*c == '/') {
            *c = '\\';
        }
    }

    if (initial_dir) {
        wchar_t wide_initial_dir[kPathSize];
        MultiByteToWideChar(CP_UTF8, 0, initial_dir_abs_path, -1, wide_initial_dir, kPathSize);
        ofn.lpstrInitialDir = wide_initial_dir;
    } else {
        ofn.lpstrInitialDir = NULL;
    }

    // Open "Open..." dialogue box
    BOOL selected = GetOpenFileNameW(&ofn);

    // Restore initial working directory
    _wchdir(initial_working_directory);

    if (!selected) {
        return NO_SELECTION;
    } else {
        for (size_t i = 0; i < MAX_PATH; ++i) {
            if (szFileName[i] == '\\')
                szFileName[i] = '/';
            else if (szFileName[i] == '\0')
                break;
        }
        int target_size = WideCharToMultiByte(CP_UTF8, 0, szFileName, -1, NULL, NULL, NULL, NULL);
        if (target_size > PATH_BUFFER_SIZE) {
            return USER_BUFFER_TOO_SMALL;
        }
        WideCharToMultiByte(CP_UTF8, 0, szFileName, -1, path_buffer, PATH_BUFFER_SIZE, NULL, NULL);
        return NO_ERR;
    }
#endif
}

DialogErr Dialog::writeFile(const char* extension, int extension_count, const char* initial_dir, char* path_buffer, int PATH_BUFFER_SIZE) {
    if (has_valid_dialogues == false) {
        LOGE << "Dialogue system was not initialized" << std::endl;
        return UNKNOWN_ERR;
    }

#if PLATFORM_UNIX
#ifndef __APPLE__
    GtkWidget* dialog;
    DialogErr err;

    dialog = gtk_file_chooser_dialog_new("Save File",
                                         NULL,
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                         NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (true)  // user_edited_a_new_document)
    {
        if (initial_dir) {
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), initial_dir);
        }

        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "");
    } else {
        if (initial_dir) {
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), initial_dir);
        }
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        int target_size = snprintf(path_buffer, PATH_BUFFER_SIZE, "%s", filename);
        if ((int)strlen(path_buffer) < target_size) {
            err = USER_BUFFER_TOO_SMALL;
        }
        err = NO_ERR;

        g_free(filename);
    } else {
        err = NO_SELECTION;
    }

    gtk_widget_hide(dialog);
    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) gtk_main_iteration();
    return err;
#else
    UIShowCursor(true);

    char* filename = NULL;
    DialogErr err = OsFileDialogsMac::SaveDialog(extension, initial_dir, &filename);

    if (err == NO_ERR) {
        if (filename != NULL) {
            int target_size = snprintf(path_buffer, PATH_BUFFER_SIZE, "%s", filename);

            if ((int)strlen(path_buffer) < target_size) {
                err = USER_BUFFER_TOO_SMALL;
            }

            free(filename);
        } else {
            err = NO_SELECTION;  // TODO: Is this the right error?
        }
    }

    UIShowCursor(false);

    return err;
#endif
#else
    // Get the initial home directory
    wchar_t initial_working_directory[kPathSize];
    if (!_wgetcwd(initial_working_directory, kPathSize)) {
        return GET_CWD_FAILED;
    }

    wchar_t wide_extension[kPathSize];
    size_t length = 0;
    for (int i = 0; i < extension_count; ++i) {
        length += strlen(&extension[length]) + 1;
    }

    MultiByteToWideChar(CP_UTF8, 0, extension, (int)length, wide_extension, kPathSize);

    // Create string specifying file types that can be opened
    wchar_t buffer[kPathSize];
    DialogErr err = SetUpWindowsFilterString(buffer, (int)length, kPathSize, wide_extension);
    if (err != NO_ERR) {
        return err;
    }
    // Set up file name structure
    OPENFILENAMEW ofn;
    wchar_t szFileName[MAX_PATH] = L"";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = buffer;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = wide_extension;

    wchar_t wide_initial_dir[kPathSize];
    length = MultiByteToWideChar(CP_UTF8, 0, initial_dir, -1, NULL, NULL);
    if (length > kPathSize)
        return INTERNAL_BUFFER_TOO_SMALL;
    MultiByteToWideChar(CP_UTF8, 0, initial_dir, -1, wide_initial_dir, kPathSize);

    wchar_t full_path[kPathSize];
    if (initial_dir) {
        err = FormatFullPath(full_path, kPathSize, wide_initial_dir);
        if (err != NO_ERR) {
            return err;
        }
        ofn.lpstrInitialDir = full_path;
    } else {
        ofn.lpstrInitialDir = NULL;
    }

    // Open "Save as..." dialogue box
    BOOL selected = GetSaveFileNameW(&ofn);

    // Restore the home directory
    _wchdir(initial_working_directory);

    if (!selected) {
        return NO_SELECTION;
    } else {
        int target_size = WideCharToMultiByte(CP_UTF8, 0, szFileName, -1, NULL, NULL, NULL, NULL);
        if (target_size > PATH_BUFFER_SIZE) {
            return USER_BUFFER_TOO_SMALL;
        }
        WideCharToMultiByte(CP_UTF8, 0, szFileName, -1, path_buffer, PATH_BUFFER_SIZE, NULL, NULL);
        return NO_ERR;
    }
    return NO_ERR;
#endif
}
