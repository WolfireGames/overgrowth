//-----------------------------------------------------------------------------
//           Name: openal.h
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

#ifdef __APPLE__
    #include <OpenAL/alc.h>
    #include <OpenAL/al.h>
#else
    #include <alc.h>
    #include <al.h>
#endif

inline const char* alErrString( const ALenum& e )
{
    switch( e )
    {
        case AL_NO_ERROR:            return "There is not currently an error";
        case AL_INVALID_NAME:        return "A bad name (ID) was passed to an OpenAL function";
        case AL_INVALID_ENUM:        return "An invalid enum value was passed to an OpenAL function";
        case AL_INVALID_VALUE:       return "An invalid value was passed to an OpenAL function";
        case AL_INVALID_OPERATION:   return "The requested operation is not valid";
        case AL_OUT_OF_MEMORY:       return "The requested operation resulted in OpenAL running out of memory";
    }

    return "Unknown error";
}

inline const char* alcErrString( const ALCenum& e )
{
    switch( e )
    {
        case ALC_NO_ERROR:          return "There is not currently an error."; 
        case ALC_INVALID_DEVICE:    return "A bad device was passed to an OpenAL function."; 
        case ALC_INVALID_CONTEXT:   return "A bad context was passed to an OpenAL function."; 
        case ALC_INVALID_ENUM:      return "An unknown enum value was passed to an OpenAL function."; 
        case ALC_INVALID_VALUE:     return "An invalid value was passed to an OpenAL function."; 
        case ALC_OUT_OF_MEMORY:     return "The requested operation resulted in OpenAL running out of memory ."; 
        default:                    return "Unknown error";
    }
}
