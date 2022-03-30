/************************************************************************************

PublicHeader:   OVR_CAPI_Util.c
Copyright   :   Copyright 2014-2016 Oculus VR, LLC All Rights reserved.

Licensed under the Oculus VR Rift SDK License Version 3.3 (the "License"); 
you may not use the Oculus VR Rift SDK except in compliance with the License, 
which is provided at the time of installation or download, or which 
otherwise accompanies this software in either electronic or hard copy form.

You may obtain a copy of the License at

http://www.oculusvr.com/licenses/LICENSE-3.3 

Unless required by applicable law or agreed to in writing, the Oculus VR SDK 
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*************************************************************************************/


#include <Extras/OVR_CAPI_Util.h>
#include <Extras/OVR_StereoProjection.h>

#include <algorithm>
#include <limits.h>
#include <memory>

#if defined(_MSC_VER) && _MSC_VER < 1800 // MSVC < 2013
    #define round(dbl) (dbl) >= 0.0 ? (int)((dbl) + 0.5) : (((dbl) - (double)(int)(dbl)) <= -0.5 ? (int)(dbl) : (int)((dbl) - 0.5))
#endif


#if defined(_MSC_VER)
    #include <emmintrin.h>
    #pragma intrinsic(_mm_pause)
#endif

#if defined(_WIN32)
    // Prevents <Windows.h> from defining min() and max() macro symbols.
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif

    #include <windows.h>
#endif



// Used to generate projection from ovrEyeDesc::Fov
OVR_PUBLIC_FUNCTION(ovrMatrix4f) ovrMatrix4f_Projection(
    ovrFovPort fov, float znear, float zfar, unsigned int projectionModFlags)
{
    bool leftHanded     = (projectionModFlags & ovrProjection_LeftHanded) > 0;
    bool flipZ          = (projectionModFlags & ovrProjection_FarLessThanNear) > 0;
    bool farAtInfinity  = (projectionModFlags & ovrProjection_FarClipAtInfinity) > 0;
    bool isOpenGL       = (projectionModFlags & ovrProjection_ClipRangeOpenGL) > 0;

    // TODO: Pass in correct eye to CreateProjection if we want to support canted displays from CAPI
    return OVR::CreateProjection(leftHanded , isOpenGL, fov, OVR::StereoEye_Center, znear, zfar, flipZ, farAtInfinity);
}

OVR_PUBLIC_FUNCTION(ovrTimewarpProjectionDesc) ovrTimewarpProjectionDesc_FromProjection(
    ovrMatrix4f Projection, unsigned int projectionModFlags)
{
    ovrTimewarpProjectionDesc res;
    res.Projection22 = Projection.M[2][2];
    res.Projection23 = Projection.M[2][3];
    res.Projection32 = Projection.M[3][2];

    if ((res.Projection32 != 1.0f) && (res.Projection32 != -1.0f))
    {
        // This is a very strange projection matrix, and probably won't work.
        // If you need it to work, please contact Oculus and let us know your usage scenario.
    }

    if ( ( projectionModFlags & ovrProjection_ClipRangeOpenGL ) != 0 )
    {
        // Internally we use the D3D range of [0,+w] not the OGL one of [-w,+w], so we need to convert one to the other.
        // Note that the values in the depth buffer, and the actual linear depth we want is the same for both APIs,
        // the difference is purely in the values inside the projection matrix.

        // D3D does this:
        // depthBuffer =             ( ProjD3D.M[2][2] * linearDepth + ProjD3D.M[2][3] ) / ( linearDepth * ProjD3D.M[3][2] );
        // OGL does this:
        // depthBuffer = 0.5 + 0.5 * ( ProjOGL.M[2][2] * linearDepth + ProjOGL.M[2][3] ) / ( linearDepth * ProjOGL.M[3][2] );

        // Therefore:
        // ProjD3D.M[2][2] = 0.5 * ( ProjOGL.M[2][2] + ProjOGL.M[3][2] );
        // ProjD3D.M[2][3] = 0.5 *   ProjOGL.M[2][3];
        // ProjD3D.M[3][2] =         ProjOGL.M[3][2];

        res.Projection22 = 0.5f * ( Projection.M[2][2] + Projection.M[3][2] );
        res.Projection23 = 0.5f *   Projection.M[2][3];
        res.Projection32 =          Projection.M[3][2];
    }
    return res;
}

OVR_PUBLIC_FUNCTION(ovrMatrix4f) ovrMatrix4f_OrthoSubProjection(
    ovrMatrix4f projection, ovrVector2f orthoScale,
    float orthoDistance, float hmdToEyeOffsetX)
{
    ovrMatrix4f ortho;
    // Negative sign is correct!
    // If the eye is offset to the left, then the ortho view needs to be offset to the right relative to the camera.
    float orthoHorizontalOffset = -hmdToEyeOffsetX / orthoDistance;

    /*
    // Current projection maps real-world vector (x,y,1) to the RT.
    // We want to find the projection that maps the range [-FovPixels/2,FovPixels/2] to
    // the physical [-orthoHalfFov,orthoHalfFov]
    // Note moving the offset from M[0][2]+M[1][2] to M[0][3]+M[1][3] - this means
    // we don't have to feed in Z=1 all the time.
    // The horizontal offset math is a little hinky because the destination is
    // actually [-orthoHalfFov+orthoHorizontalOffset,orthoHalfFov+orthoHorizontalOffset]
    // So we need to first map [-FovPixels/2,FovPixels/2] to
    //                         [-orthoHalfFov+orthoHorizontalOffset,orthoHalfFov+orthoHorizontalOffset]:
    // x1 = x0 * orthoHalfFov/(FovPixels/2) + orthoHorizontalOffset;
    //    = x0 * 2*orthoHalfFov/FovPixels + orthoHorizontalOffset;
    // But then we need the same mapping as the existing projection matrix, i.e.
    // x2 = x1 * Projection.M[0][0] + Projection.M[0][2];
    //    = x0 * (2*orthoHalfFov/FovPixels + orthoHorizontalOffset) * Projection.M[0][0] + Projection.M[0][2];
    //    = x0 * Projection.M[0][0]*2*orthoHalfFov/FovPixels +
    //      orthoHorizontalOffset*Projection.M[0][0] + Projection.M[0][2];
    // So in the new projection matrix we need to scale by Projection.M[0][0]*2*orthoHalfFov/FovPixels and
    // offset by orthoHorizontalOffset*Projection.M[0][0] + Projection.M[0][2].
    */

    ortho.M[0][0] = projection.M[0][0] * orthoScale.x;
    ortho.M[0][1] = 0.0f;
    ortho.M[0][2] = 0.0f;
    ortho.M[0][3] = -projection.M[0][2] + ( orthoHorizontalOffset * projection.M[0][0] );

    ortho.M[1][0] = 0.0f;
    ortho.M[1][1] = -projection.M[1][1] * orthoScale.y;       /* Note sign flip (text rendering uses Y=down). */
    ortho.M[1][2] = 0.0f;
    ortho.M[1][3] = -projection.M[1][2];

    ortho.M[2][0] = 0.0f;
    ortho.M[2][1] = 0.0f;
    ortho.M[2][2] = 0.0f;
    ortho.M[2][3] = 0.0f;

    /* No perspective correction for ortho. */
    ortho.M[3][0] = 0.0f;
    ortho.M[3][1] = 0.0f;
    ortho.M[3][2] = 0.0f;
    ortho.M[3][3] = 1.0f;

    return ortho;
}


OVR_PUBLIC_FUNCTION(void) ovr_CalcEyePoses(ovrPosef headPose,
    const ovrVector3f hmdToEyeOffset[2],
    ovrPosef outEyePoses[2])
{
    if (!hmdToEyeOffset || !outEyePoses)
    {
        return;
    }

    using OVR::Posef;
    using OVR::Vector3f;

    // Currently hmdToEyeOffset is only a 3D vector
    outEyePoses[0] = Posef(headPose.Orientation, ((Posef)headPose).Apply((Vector3f)hmdToEyeOffset[0]));
    outEyePoses[1] = Posef(headPose.Orientation, ((Posef)headPose).Apply((Vector3f)hmdToEyeOffset[1]));
}


OVR_PUBLIC_FUNCTION(void) ovr_GetEyePoses(ovrSession session, long long frameIndex, ovrBool latencyMarker,
    const ovrVector3f hmdToEyeOffset[2],    
    ovrPosef outEyePoses[2],
    double* outSensorSampleTime)
{
    double frameTime = ovr_GetPredictedDisplayTime(session, frameIndex);
    ovrTrackingState trackingState = ovr_GetTrackingState(session, frameTime, latencyMarker);
    ovr_CalcEyePoses(trackingState.HeadPose.ThePose, hmdToEyeOffset, outEyePoses);

    if (outSensorSampleTime != NULL)
    {
        *outSensorSampleTime = ovr_GetTimeInSeconds();
    }
}

OVR_PUBLIC_FUNCTION(ovrDetectResult) ovr_Detect(int timeoutMilliseconds)
{
    // Initially we assume everything is not running.
    ovrDetectResult result;
    result.IsOculusHMDConnected = ovrFalse;
    result.IsOculusServiceRunning = ovrFalse;

#if defined(_WIN32)
    // Attempt to open the named event.
    HANDLE hServiceEvent = ::OpenEventW(SYNCHRONIZE, FALSE, OVR_HMD_CONNECTED_EVENT_NAME);

    // If event exists,
    if (hServiceEvent != NULL)
    {
        // This indicates that the Oculus Runtime is installed and running.
        result.IsOculusServiceRunning = ovrTrue;

        // Poll for event state.
        DWORD objectResult = ::WaitForSingleObject(hServiceEvent, timeoutMilliseconds);

        // If the event is signaled,
        if (objectResult == WAIT_OBJECT_0)
        {
            // This indicates that the Oculus HMD is connected.
            result.IsOculusHMDConnected = ovrTrue;
        }

        ::CloseHandle(hServiceEvent);
    }
#endif // _WIN32


    return result;
}

OVR_PUBLIC_FUNCTION(void) ovrPosef_FlipHandedness(const ovrPosef* inPose, ovrPosef* outPose)
{
    outPose->Orientation.x = -inPose->Orientation.x;
    outPose->Orientation.y = inPose->Orientation.y;
    outPose->Orientation.z = inPose->Orientation.z;
    outPose->Orientation.w = -inPose->Orientation.w;

    outPose->Position.x = -inPose->Position.x;
    outPose->Position.y = inPose->Position.y;
    outPose->Position.z = inPose->Position.z;
}

static float wavPcmBytesToFloat(const void* data, int32_t sizeInBits, bool swapBytes) {
    // TODO Support big endian
    (void)swapBytes;
    
    // There's not a strong standard to convert 8/16/32b PCM to float.
    // For 16b: MSDN says range is [-32760, 32760], Pyton Scipy uses [-32767, 32767] and Audacity outputs the full range [-32768, 32767].
    // We use the same range on both sides and clamp to [-1, 1].

    float result = 0.0f;
    if (sizeInBits == 8)
        // uint8_t is a special case, unsigned where 128 is zero
        result = (*((uint8_t*)data) / (float)UCHAR_MAX) * 2.0f - 1.0f;
    else if (sizeInBits == 16)
        result = *((int16_t*)data) / (float)SHRT_MAX;
    //else if (sizeInBits == 24) {
    //    int value = data[0] | data[1] << 8 | data[2] << 16; // Need consider 2's complement
    //    return value / 8388607.0f;
    //}
    else if (sizeInBits == 32)
        result = *((int32_t*)data) / (float)INT_MAX;

    return std::max(-1.0f, result);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GenHapticsFromAudioData(ovrHapticsClip* outHapticsClip, const ovrAudioChannelData* audioChannel, ovrHapticsGenMode genMode)
{
    if (!outHapticsClip || !audioChannel || genMode != ovrHapticsGenMode_PointSample)
        return ovrError_InvalidParameter;
    // Validate audio channel
    if (audioChannel->Frequency <= 0 || audioChannel->SamplesCount <= 0 || audioChannel->Samples == NULL)
        return ovrError_InvalidParameter;

    const int32_t kHapticsFrequency = 320;
    const int32_t kHapticsMaxAmplitude = 255;
    float samplesPerStep = audioChannel->Frequency / (float)kHapticsFrequency;
    int32_t hapticsSampleCount = (int32_t)ceil(audioChannel->SamplesCount / samplesPerStep);
    
    uint8_t* hapticsSamples = new uint8_t[hapticsSampleCount];
    for (int32_t i = 0; i < hapticsSampleCount; ++i)
    {
        float sample = audioChannel->Samples[(int32_t)(i * samplesPerStep)];
        uint8_t hapticSample = (uint8_t)std::min(UCHAR_MAX, (int)round(abs(sample) * kHapticsMaxAmplitude));
        hapticsSamples[i] = hapticSample;
    }

    outHapticsClip->Samples = hapticsSamples;
    outHapticsClip->SamplesCount = hapticsSampleCount;

    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_ReadWavFromBuffer(ovrAudioChannelData* outAudioChannel, const void* inputData, int dataSizeInBytes, int stereoChannelToUse)
{
    // We don't support any format other than PCM and IEEE Float
    enum WavFormats {
        kWavFormatUnknown     = 0x0000,
        kWavFormatLPCM        = 0x0001,
        kWavFormatFloatIEEE   = 0x0003,
        kWavFormatExtensible  = 0xFFFE
    };

    struct WavHeader {
        char RiffId[4];             // "RIFF" = little-endian, "RIFX" = big-endian
        int32_t Size;               // 4 + (8 + FmtChunkSize) + (8 + DataChunkSize)
        char WavId[4];              // Must be "WAVE"

        char FmtChunckId[4];        // Must be "fmt "
        uint32_t FmtChunkSize;      // Remaining size of this chunk (16B)
        uint16_t Format;            // WavFormats: PCM or Float supported
        uint16_t Channels;          // 1 = Mono, 2 = Stereo
        uint32_t SampleRate;        // e.g. 44100
        uint32_t BytesPerSec;       // SampleRate * BytesPerBlock
        uint16_t BytesPerBlock;     // (NumChannels * BitsPerSample/8)
        uint16_t BitsPerSample;     // 8, 16, 32

        char DataChunckId[4];       // Must be "data"
        uint32_t DataChunkSize;     // Remaining size of this chunk
    };

    const int32_t kMinWavFileSize = sizeof(WavHeader) + 1;
    if (!outAudioChannel || !inputData || dataSizeInBytes < kMinWavFileSize)
        return ovrError_InvalidParameter;

    WavHeader* header = (WavHeader*)inputData;
    uint8_t* data = (uint8_t*)inputData + sizeof(WavHeader);

    // Validate
    const char* wavId = header->RiffId;
    // TODO We need to support RIFX when supporting big endian formats
    //bool isValidWav = (wavId[0] == 'R' && wavId[1] == 'I' && wavId[2] == 'F' && (wavId[3] == 'F' || wavId[3] == 'X')) &&
    bool isValidWav = (wavId[0] == 'R' && wavId[1] == 'I' && wavId[2] == 'F' && wavId[3] == 'F') &&
        memcmp(header->WavId, "WAVE", 4) == 0;
    bool hasValidChunks = memcmp(header->FmtChunckId, "fmt ", 4) == 0 && 
        memcmp(header->DataChunckId, "data ", 4) == 0;
    if (!isValidWav || !hasValidChunks) {
        return ovrError_InvalidOperation;
    }

    // We only support PCM
    bool isSupported = (header->Format == kWavFormatLPCM || header->Format == kWavFormatFloatIEEE) &&
        (header->Channels == 1 || header->Channels == 2) && 
        (header->BitsPerSample == 8 || header->BitsPerSample == 16 || header->BitsPerSample == 32);
    if (!isSupported) {
        return ovrError_Unsupported;
    }

    // Channel selection
    bool useSecondChannel = (header->Channels == 2 && stereoChannelToUse == 1);
    int32_t channelOffset = (useSecondChannel)? header->BytesPerBlock / 2  : 0;
    
    // TODO Support big-endian
    int32_t blockCount = header->DataChunkSize / header->BytesPerBlock;
    float* samples = new float[blockCount];

    for (int32_t i = 0; i < blockCount; i++) {
        int32_t dataIndex = i * header->BytesPerBlock;
        uint8_t* dataPtr = &data[dataIndex + channelOffset];
        float sample = (header->Format == kWavFormatLPCM)?
            wavPcmBytesToFloat(dataPtr, header->BitsPerSample, false) :
            *(float*)dataPtr;
        
        samples[i] = sample;
    }

    // Output
    outAudioChannel->Samples = samples;
    outAudioChannel->SamplesCount = blockCount;
    outAudioChannel->Frequency = header->SampleRate;

    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(void) ovr_ReleaseAudioChannelData(ovrAudioChannelData* audioChannel)
{
    if (audioChannel != NULL && audioChannel->Samples != NULL) {
        delete[] audioChannel->Samples;
        memset(audioChannel, 0, sizeof(ovrAudioChannelData));
    }
}

OVR_PUBLIC_FUNCTION(void) ovr_ReleaseHapticsClip(ovrHapticsClip* hapticsClip)
{
    if (hapticsClip != NULL && hapticsClip->Samples != NULL) {
        delete[] hapticsClip->Samples;
        memset(hapticsClip, 0, sizeof(ovrHapticsClip));
    }
}
