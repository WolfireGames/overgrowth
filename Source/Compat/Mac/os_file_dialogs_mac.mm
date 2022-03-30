//-----------------------------------------------------------------------------
//           Name: os_file_dialogs_mac.mm
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
#include <Internal/dialogues.h>

#include <AppKit/AppKit.h>

namespace OsFileDialogsMac {

Dialog::DialogErr OpenDialog(const char* filter_list, const char* default_path, char** output_path) {
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    NSWindow* key_window = [[NSApplication sharedApplication] keyWindow];
    NSOpenPanel* dialog = [NSOpenPanel openPanel];
    [dialog setAllowsMultipleSelection: NO];

    if(filter_list && strlen(filter_list) != 0) {
        NSMutableArray* allowed_extensions_list = [[NSMutableArray alloc] init];

        char current_ext[256] = { 0 };
        char* current_ext_char = &current_ext[0];
        size_t filter_list_len = strlen(filter_list);

        for(size_t i = 0; i < filter_list_len + 1; ++i) {
            // TODO: Error check that we're not past the current_ext buffer length
            *current_ext_char = filter_list[i];
            ++current_ext_char;

            if(filter_list[i] == '\0') {
                NSString* current_ext_ns_string = [NSString stringWithUTF8String: current_ext];
                [allowed_extensions_list addObject: current_ext_ns_string];

                current_ext_char = &current_ext[0];
                *current_ext_char = '\0';
            }
        }

        NSArray* allowed_file_types = [NSArray arrayWithArray: allowed_extensions_list];
        [allowed_extensions_list release];

        if([allowed_file_types count] != 0) {
            [dialog setAllowedFileTypes: allowed_file_types];
        }
    }

    if(default_path && strlen(default_path) != 0) {
        NSString* default_path_string = [NSString stringWithUTF8String: default_path];
        NSURL* path_url = [NSURL fileURLWithPath: default_path_string isDirectory: YES];
        [dialog setDirectoryURL: path_url];
    }

    Dialog::DialogErr result = Dialog::DialogErr::NO_SELECTION;

    if([dialog runModal] == NSModalResponseOK) {
        NSURL* path_url = [dialog URL];
        const char* utf8_path = [[path_url path] UTF8String];

        size_t len = strlen(utf8_path);
        *output_path = (char*)malloc(len + 1);

        if(!*output_path) {
            [pool release];
            [key_window makeKeyAndOrderFront: nil];
            return Dialog::DialogErr::INTERNAL_BUFFER_TOO_SMALL;
        }

        memcpy(*output_path, utf8_path, len + 1);
        result = Dialog::DialogErr::NO_ERR;
    }

    [pool release];
    [key_window makeKeyAndOrderFront: nil];

    return result;
}

Dialog::DialogErr SaveDialog(const char* filter_list, const char* default_path, char** output_path) {
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    NSWindow* key_window = [[NSApplication sharedApplication] keyWindow];
    NSSavePanel *dialog = [NSSavePanel savePanel];
    [dialog setExtensionHidden: NO];

    if(filter_list && strlen(filter_list) != 0) {
        NSMutableArray* allowed_extensions_list = [[NSMutableArray alloc] init];

        char current_ext[256] = { 0 };
        char* current_ext_char = &current_ext[0];
        size_t filter_list_len = strlen(filter_list);

        for(size_t i = 0; i < filter_list_len + 1; ++i) {
            // TODO: Error check that we're not past the current_ext buffer length
            *current_ext_char = filter_list[i];
            ++current_ext_char;

            if(filter_list[i] == '\0') {
                NSString* current_ext_ns_string = [NSString stringWithUTF8String: current_ext];
                [allowed_extensions_list addObject: current_ext_ns_string];

                current_ext_char = &current_ext[0];
                *current_ext_char = '\0';
            }
        }

        NSArray* allowed_file_types = [NSArray arrayWithArray: allowed_extensions_list];
        [allowed_extensions_list release];

        if([allowed_file_types count] != 0) {
            [dialog setAllowedFileTypes: allowed_file_types];
        }
    }

    if(default_path && strlen(default_path) != 0) {
        NSString* default_path_string = [NSString stringWithUTF8String: default_path];
        NSURL* path_url = [NSURL fileURLWithPath: default_path_string isDirectory: YES];
        [dialog setDirectoryURL: path_url];
    }

    Dialog::DialogErr result = Dialog::DialogErr::NO_SELECTION;

    if([dialog runModal] == NSModalResponseOK) {
        NSURL* path_url = [dialog URL];
        const char* utf8_path = [[path_url path] UTF8String];

        size_t len = [path_url.path lengthOfBytesUsingEncoding: NSUTF8StringEncoding];
        *output_path = (char*)malloc(len + 1);

        if(!*output_path) {
            [pool release];
            [key_window makeKeyAndOrderFront: nil];
            return Dialog::DialogErr::INTERNAL_BUFFER_TOO_SMALL;
        }

        memcpy(*output_path, utf8_path, len + 1);
        result = Dialog::DialogErr::NO_ERR;
    }

    [pool release];
    [key_window makeKeyAndOrderFront: nil];

    return result;
}

};  // namespace OsFileDialogsMac
