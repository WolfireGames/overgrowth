//-----------------------------------------------------------------------------
//           Name: mac_hardware_info.cpp
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
#include <Compat/hardware_info.h>
#include <Memory/allocation.h>

#include <opengl.h>

#include <string>

unsigned GetDriverVersion(GLVendor vendor) {
    // Mac driver versions are at the end of the GL_VERSION string, e.g.:
    //"2.1 NVIDIA-1.6.6"
    std::string driver_string = (const char *)glGetString(GL_VERSION);

    // Remove everything before dash
    unsigned final_dash_index = driver_string.rfind('-');
    driver_string = driver_string.substr(final_dash_index + 1);

    // Remove dots
    driver_string.erase(
        std::remove(driver_string.begin(), driver_string.end(), '.'),
        driver_string.end());

    // Convert remaining string into unsigned int and return
    unsigned driver_version = atoi(driver_string.c_str());
    return driver_version;
}

#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/IOKitLib.h>

int vramSize(long **vsArray) {
    CGError err = CGDisplayNoErr;
    unsigned int i = 0;
    io_service_t *dspPorts = NULL;
    CGDirectDisplayID *displays = NULL;
    CGDisplayCount dspCount = 0;
    CFTypeRef typeCode;

    // How many active displays do we have?
    err = CGGetActiveDisplayList(0, NULL, &dspCount);

    // Allocate enough memory to hold all the display IDs we have
    displays = (CGDirectDisplayID *)calloc((size_t)dspCount, sizeof(CGDirectDisplayID));
    // Allocate enough memory for the number of displays we're asking about
    *vsArray = (long int *)calloc((size_t)dspCount, sizeof(long));
    // Allocate memory for our service ports
    dspPorts = (io_service_t *)calloc((size_t)dspCount, sizeof(io_service_t));

    // Get the list of active displays
    err = CGGetActiveDisplayList(dspCount,
                                 displays,
                                 &dspCount);

    // Now we iterate through them
    for (i = 0; i < dspCount; i++) {
        // Get the service port for the display
        dspPorts[i] = CGDisplayIOServicePort(displays[i]);
        // Ask IOKit for the VRAM size property
        typeCode = IORegistryEntryCreateCFProperty(dspPorts[i],
                                                   CFSTR(kIOFBMemorySizeKey),
                                                   kCFAllocatorDefault,
                                                   kNilOptions);

        // Ensure we have valid data from IOKit
        if (typeCode && CFGetTypeID(typeCode) == CFNumberGetTypeID()) {
            // If so, convert the CFNumber into a plain unsigned long
            CFNumberGetValue((const __CFNumber *)typeCode, kCFNumberSInt32Type, vsArray[i]);
            if (typeCode)
                CFRelease(typeCode);
        }
    }
    OG_FREE(dspPorts);
    // Return the total number of displays we found
    return (int)dspCount;
}

long MACOS_GetVramSize(int displayNumber) {
    long displaySize = -1;
    long *vramArray;
    int dispCount = vramSize(&vramArray);
    if (displayNumber < dispCount && displayNumber >= 0) {
        displaySize = vramArray[displayNumber];
    }
    OG_FREE(vramArray);
    return displaySize;
}
