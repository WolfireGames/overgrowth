//-----------------------------------------------------------------------------
//           Name: win_hardware_info.cpp
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

#include <Logging/logdata.h>

#include <nvapi.h>
#include <opengl.h>

#include <string>

unsigned GetWindowsNvidiaDriverInfo(){
    NvAPI_Status status = NvAPI_Initialize();
    NvAPI_ShortString msg;
    if (status != NVAPI_OK) {
        NvAPI_GetErrorMessage (status, msg);
        LOGE << "Cannot initialize NvAPI: " << msg << std::endl;
        return 0;
    }

    NvDisplayHandle hDisp;
    status = NvAPI_EnumNvidiaDisplayHandle (0, &hDisp);
    if (status != NVAPI_OK) {
        NvAPI_GetErrorMessage (status, msg);
        LOGE << "Cannot initialize NvAPI: " <<  msg << std::endl;
        return 0;
    }

    NV_DISPLAY_DRIVER_VERSION ver;
    memset (&ver, 0, sizeof (NV_DISPLAY_DRIVER_VERSION));
    ver.version = NV_DISPLAY_DRIVER_VERSION_VER;
    status = NvAPI_GetDisplayDriverVersion (hDisp, &ver);
    if (status != NVAPI_OK) {
        NvAPI_GetErrorMessage (status, msg);
        LOGE << "Cannot initialize NvAPI: " <<  msg << std::endl;
        return 0;
    }

    return ver.drvVersion;
}

unsigned GetDriverVersion(GLVendor vendor) {
    if(vendor == _nvidia){
        return GetWindowsNvidiaDriverInfo();
    } else if(vendor == _ati){
        //ATI driver versions are the end of the GL_VERSION string, e.g.:
        //2.1.8577 (8577 is the driver version)
        std::string driver_string = (const char*)glGetString(GL_VERSION);

        //Remove everything before the final dot
        unsigned final_dot_index = driver_string.rfind('.');
        driver_string = driver_string.substr(final_dot_index+1);

        //What remains is the driver string. Return as unsigned int
        return atoi(driver_string.c_str());
    } else {
        return 0;
    }
}
