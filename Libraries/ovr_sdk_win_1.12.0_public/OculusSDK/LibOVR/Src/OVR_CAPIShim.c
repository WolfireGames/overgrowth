/************************************************************************************

Filename    :   OVR_CAPIShim.c
Content     :   CAPI DLL user library
Created     :   November 20, 2014
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

************************************************************************************/


#include "OVR_CAPI.h"
#include "OVR_Version.h"
#include "OVR_ErrorCode.h"
#include "OVR_CAPI_Prototypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#if defined(_WIN32)
    #if defined(_MSC_VER)
        #pragma warning(push, 0)
    #endif
    #include <Windows.h>
    #if defined(_MSC_VER)
        #pragma warning(pop)
    #endif

    #include "../Include/OVR_CAPI_D3D.h"
#else
    #if defined(__APPLE__)
        #include <mach-o/dyld.h>
        #include <sys/syslimits.h>
        #include <libgen.h>
        #include <pwd.h>
        #include <unistd.h>
    #endif
    #include <dlfcn.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif
#include "../Include/OVR_CAPI_GL.h"


#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4996) // 'getenv': This function or variable may be unsafe.
#endif

static const uint8_t OculusSDKUniqueIdentifier[] =
{
    0x9E, 0xB2, 0x0B, 0x1A, 0xB7, 0x97, 0x09, 0x20, 0xE0, 0xFB, 0x83, 0xED, 0xF8, 0x33, 0x5A, 0xEB,
    0x80, 0x4D, 0x8E, 0x92, 0x20, 0x69, 0x13, 0x56, 0xB4, 0xBB, 0xC4, 0x85, 0xA7, 0x9E, 0xA4, 0xFE,
    OVR_MAJOR_VERSION, OVR_MINOR_VERSION, OVR_PATCH_VERSION
};

static const uint8_t OculusSDKUniqueIdentifierXORResult = 0xcb;


// -----------------------------------------------------------------------------------
// ***** OVR_ENABLE_DEVELOPER_SEARCH
//
// If defined then our shared library loading code searches for developer build
// directories.
//
#if !defined(OVR_ENABLE_DEVELOPER_SEARCH)
#endif


// -----------------------------------------------------------------------------------
// ***** OVR_BUILD_DEBUG
//
// Defines OVR_BUILD_DEBUG when the compiler default debug preprocessor is set.
//
// If you want to control the behavior of these flags, then explicitly define
// either -DOVR_BUILD_RELEASE or -DOVR_BUILD_DEBUG in the compiler arguments.

#if !defined(OVR_BUILD_DEBUG) && !defined(OVR_BUILD_RELEASE)
    #if defined(_MSC_VER)
        #if defined(_DEBUG)
            #define OVR_BUILD_DEBUG
        #endif
    #else
        #if defined(DEBUG)
            #define OVR_BUILD_DEBUG
        #endif
    #endif
#endif


//-----------------------------------------------------------------------------------
// ***** FilePathCharType, ModuleHandleType, ModuleFunctionType
//
#if defined(_WIN32)                                       // We need to use wchar_t on Microsoft platforms, as that's the native file system character type.
    #define FilePathCharType       wchar_t                // #define instead of typedef because debuggers (VC++, XCode) don't recognize typedef'd types as a string type.
    typedef HMODULE                ModuleHandleType;
    typedef FARPROC                ModuleFunctionType;
#else
    #define FilePathCharType       char
    typedef void*                  ModuleHandleType;
    typedef void*                  ModuleFunctionType;
#endif

#define ModuleHandleTypeNull   ((ModuleHandleType)NULL)
#define ModuleFunctionTypeNull ((ModuleFunctionType)NULL)


//-----------------------------------------------------------------------------------
// ***** OVR_MAX_PATH
//
#if !defined(OVR_MAX_PATH)
    #if defined(_WIN32)
        #define OVR_MAX_PATH  _MAX_PATH
    #elif defined(__APPLE__)
        #define OVR_MAX_PATH  PATH_MAX
    #else
        #define OVR_MAX_PATH  1024
    #endif
#endif

#if !defined(OVR_DLSYM)
    #if defined(_WIN32)
        #define OVR_DLSYM(dlImage, name) GetProcAddress(dlImage, name)
    #else
        #define OVR_DLSYM(dlImage, name) dlsym(dlImage, name)
    #endif
#endif

static size_t OVR_strlcpy(char* dest, const char* src, size_t destsize)
{
    const char* s = src;
    size_t      n = destsize;

    if(n && --n)
    {
        do{
            if((*dest++ = *s++) == 0)
                break;
        } while(--n);
    }

    if(!n)
    {
        if(destsize)
            *dest = 0;
        while(*s++)
            { }
    }

    return (size_t)((s - src) - 1);
}


static size_t OVR_strlcat(char* dest, const char* src, size_t destsize)
{
    const size_t d = destsize ? strlen(dest) : 0;
    const size_t s = strlen(src);
    const size_t t = s + d;

    if(t < destsize)
        memcpy(dest + d, src, (s + 1) * sizeof(*src));
    else
    {
        if(destsize)
        {
            memcpy(dest + d, src, ((destsize - d) - 1) * sizeof(*src));
            dest[destsize - 1] = 0;
        }
    }

    return t;
}


#if defined(__APPLE__)
    static ovrBool OVR_strend(const char* pStr, const char* pFind, size_t strLength, size_t findLength)
    {
        if(strLength == (size_t)-1)
            strLength = strlen(pStr);
        if(findLength == (size_t)-1)
            findLength = strlen(pFind);
        if(strLength >= findLength)
            return (strcmp(pStr + strLength - findLength, pFind) == 0);
        return ovrFalse;
    }

    static ovrBool OVR_isBundleFolder(const char* filePath)
    {
        static const char* extensionArray[] = { ".app", ".bundle", ".framework", ".plugin", ".kext" };
        size_t i;

        for(i = 0; i < sizeof(extensionArray)/sizeof(extensionArray[0]); i++)
        {
            if(OVR_strend(filePath, extensionArray[i], (size_t)-1, (size_t)-1))
                return ovrTrue;
        }

        return ovrFalse;
    }
#endif


#if defined(OVR_ENABLE_DEVELOPER_SEARCH)

// Returns true if the path begins with the given prefix.
// Doesn't support non-ASCII paths, else the return value may be incorrect.
static int OVR_PathStartsWith(const FilePathCharType* path, const char* prefix)
{
    while(*prefix)
    {
        if(tolower((unsigned char)*path++) != tolower((unsigned char)*prefix++))
            return ovrFalse;
    }

    return ovrTrue;
}

#endif


static ovrBool OVR_GetCurrentWorkingDirectory(FilePathCharType* directoryPath, size_t directoryPathCapacity)
{
    #if defined(_WIN32)
        DWORD dwSize = GetCurrentDirectoryW((DWORD)directoryPathCapacity, directoryPath);

        if((dwSize > 0) && (directoryPathCapacity > 1)) // Test > 1 so we have room to possibly append a \ char.
        {
            size_t length = wcslen(directoryPath);

            if((length == 0) || ((directoryPath[length - 1] != L'\\') && (directoryPath[length - 1] != L'/')))
            {
                directoryPath[length++] = L'\\';
                directoryPath[length]   = L'\0';
            }

            return ovrTrue;
        }

    #else
        char* cwd = getcwd(directoryPath, directoryPathCapacity);

        if(cwd && directoryPath[0] && (directoryPathCapacity > 1)) // Test > 1 so we have room to possibly append a / char.
        {
            size_t length = strlen(directoryPath);

            if((length == 0) || (directoryPath[length - 1] != '/'))
            {
                directoryPath[length++] = '/';
                directoryPath[length]   = '\0';
            }

            return ovrTrue;
        }
    #endif

    if(directoryPathCapacity > 0)
        directoryPath[0] = '\0';

    return ovrFalse;
}


// The appContainer argument is specific currently to only Macintosh. If true and the application is a .app bundle then it returns the
// location of the bundle and not the path to the executable within the bundle. Else return the path to the executable binary itself.
// The moduleHandle refers to the relevant dynamic (a.k.a. shared) library. The main executable is the main module, and each of the shared
// libraries is a module. This way you can specify that you want to know the directory of the given shared library, which may be different
// from the main executable. If the moduleHandle is NULL then the current application module is used.
static ovrBool OVR_GetCurrentApplicationDirectory(FilePathCharType* directoryPath, size_t directoryPathCapacity, ovrBool appContainer, ModuleHandleType moduleHandle)
{
    #if defined(_WIN32)
        DWORD length = GetModuleFileNameW(moduleHandle, directoryPath, (DWORD)directoryPathCapacity);
        DWORD pos;

        if((length != 0) && (length < (DWORD)directoryPathCapacity)) // If there wasn't an error and there was enough capacity...
        {
            for(pos = length; (pos > 0) && (directoryPath[pos] != '\\') && (directoryPath[pos] != '/'); --pos)
            {
                if((directoryPath[pos - 1] != '\\') && (directoryPath[pos - 1] != '/'))
                   directoryPath[pos - 1] = 0;
            }

            return ovrTrue;
        }

        (void)appContainer; // Not used on this platform.

    #elif defined(__APPLE__)
        uint32_t directoryPathCapacity32 = (uint32_t)directoryPathCapacity;
        int result = _NSGetExecutablePath(directoryPath, &directoryPathCapacity32);

        if(result == 0) // If success...
        {
            char realPath[OVR_MAX_PATH];

            if(realpath(directoryPath, realPath)) // realpath returns the canonicalized absolute file path.
            {
                size_t length = 0;

                if(appContainer) // If the caller wants the path to the containing bundle...
                {
                    char    containerPath[OVR_MAX_PATH];
                    ovrBool pathIsContainer;

                    OVR_strlcpy(containerPath, realPath, sizeof(containerPath));
                    pathIsContainer = OVR_isBundleFolder(containerPath);

                    while(!pathIsContainer && strncmp(containerPath, ".", OVR_MAX_PATH) && strncmp(containerPath, "/", OVR_MAX_PATH)) // While the container we're looking for is not found and while the path doesn't start with a . or /
                    {
                        OVR_strlcpy(containerPath, dirname(containerPath), sizeof(containerPath));
                        pathIsContainer = OVR_isBundleFolder(containerPath);
                    }

                    if(pathIsContainer)
                        length = OVR_strlcpy(directoryPath, containerPath, directoryPathCapacity);
                }

                if(length == 0) // If not set above in the appContainer block...
                    length = OVR_strlcpy(directoryPath, realPath, directoryPathCapacity);

                while(length-- && (directoryPath[length] != '/'))
                    directoryPath[length] = '\0'; // Strip the file name from the file path, leaving a trailing / char.

                return ovrTrue;
            }
        }

        (void)moduleHandle;  // Not used on this platform.

    #else
        ssize_t length = readlink("/proc/self/exe", directoryPath, directoryPathCapacity);
        ssize_t pos;

        if(length > 0)
        {
            for(pos = length; (pos > 0) && (directoryPath[pos] != '/'); --pos)
            {
                if(directoryPath[pos - 1] != '/')
                   directoryPath[pos - 1]  = '\0';
            }

            return ovrTrue;
        }

        (void)appContainer; // Not used on this platform.
        (void)moduleHandle;
    #endif

    if(directoryPathCapacity > 0)
        directoryPath[0] = '\0';

    return ovrFalse;
}


#if defined(_WIN32) || defined(OVR_ENABLE_DEVELOPER_SEARCH) // Used only in these cases

// Get the file path to the current module's (DLL or EXE) directory within the current process.
// Will be different from the process module handle if the current module is a DLL and is in a different directory than the EXE module.
// If successful then directoryPath will be valid and ovrTrue is returned, else directoryPath will be empty and ovrFalse is returned.
static ovrBool OVR_GetCurrentModuleDirectory(FilePathCharType* directoryPath, size_t directoryPathCapacity, ovrBool appContainer)
{
    #if defined(_WIN32)
        HMODULE hModule;
        BOOL result = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)(uintptr_t)OVR_GetCurrentModuleDirectory, &hModule);
        if(result)
            OVR_GetCurrentApplicationDirectory(directoryPath, directoryPathCapacity, ovrTrue, hModule);
        else
            directoryPath[0] = 0;

        (void)appContainer;

        return directoryPath[0] ? ovrTrue : ovrFalse;
    #else
        return OVR_GetCurrentApplicationDirectory(directoryPath, directoryPathCapacity, appContainer, NULL);
    #endif
}

#endif

#if defined(_WIN32)

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4201)
#endif

#include <Softpub.h>
#include <Wincrypt.h>

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

// Expected certificates:
#define ExpectedNumCertificates 3
typedef struct CertificateEntry_t {
    const wchar_t* Issuer;
    const wchar_t* Subject;
} CertificateEntry;

CertificateEntry NewCertificateChain[ExpectedNumCertificates] = {
    { L"DigiCert SHA2 Assured ID Code Signing CA", L"Oculus VR, LLC" },
    { L"DigiCert Assured ID Root CA", L"DigiCert SHA2 Assured ID Code Signing CA" },
    { L"DigiCert Assured ID Root CA", L"DigiCert Assured ID Root CA" },
};

#define CertificateChainCount 1
CertificateEntry* AllowedCertificateChains[CertificateChainCount] = {
    NewCertificateChain
};

typedef WINCRYPT32API
DWORD
(WINAPI *PtrCertGetNameStringW)(
    PCCERT_CONTEXT pCertContext,
    DWORD dwType,
    DWORD dwFlags,
    void *pvTypePara,
    LPWSTR pszNameString,
    DWORD cchNameString
    );
typedef LONG (WINAPI *PtrWinVerifyTrust)(HWND hwnd, GUID *pgActionID,
                                  LPVOID pWVTData);
typedef CRYPT_PROVIDER_DATA * (WINAPI *PtrWTHelperProvDataFromStateData)(HANDLE hStateData);
typedef CRYPT_PROVIDER_SGNR * (WINAPI *PtrWTHelperGetProvSignerFromChain)(
    CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner, BOOL fCounterSigner, DWORD idxCounterSigner);

PtrCertGetNameStringW m_PtrCertGetNameStringW = 0;
PtrWinVerifyTrust m_PtrWinVerifyTrust = 0;
PtrWTHelperProvDataFromStateData m_PtrWTHelperProvDataFromStateData = 0;
PtrWTHelperGetProvSignerFromChain m_PtrWTHelperGetProvSignerFromChain = 0;

typedef enum ValidateCertificateContentsResult_
{
    VCCRSuccess          =  0,
    VCCRErrorCertCount   = -1,
    VCCRErrorTrust       = -2,
    VCCRErrorValidation  = -3
} ValidateCertificateContentsResult;

static ValidateCertificateContentsResult ValidateCertificateContents(CertificateEntry* chain, CRYPT_PROVIDER_SGNR* cps)
{
    int certIndex;

    if (!cps ||
        !cps->pasCertChain ||
        cps->csCertChain != ExpectedNumCertificates)
    {
        return VCCRErrorCertCount;
    }

    for (certIndex = 0; certIndex < ExpectedNumCertificates; ++certIndex)
    {
        CRYPT_PROVIDER_CERT* pCertData = &cps->pasCertChain[certIndex];
        wchar_t subjectStr[400] = { 0 };
        wchar_t issuerStr[400] = { 0 };

        if ((pCertData->fSelfSigned && !pCertData->fTrustedRoot) ||
            pCertData->fTestCert)
        {
            return VCCRErrorTrust;
        }

        m_PtrCertGetNameStringW(
            pCertData->pCert,
            CERT_NAME_ATTR_TYPE,
            0,
            szOID_COMMON_NAME,
            subjectStr,
            ARRAYSIZE(subjectStr));

        m_PtrCertGetNameStringW(
            pCertData->pCert,
            CERT_NAME_ATTR_TYPE,
            CERT_NAME_ISSUER_FLAG,
            0,
            issuerStr,
            ARRAYSIZE(issuerStr));

        if (wcscmp(subjectStr, chain[certIndex].Subject) != 0 ||
            wcscmp(issuerStr, chain[certIndex].Issuer) != 0)
        {
            return VCCRErrorValidation;
        }
    }

    return VCCRSuccess;
}

#define OVR_SIGNING_CONVERT_PTR(ftype, fptr, procaddr) { \
        union { ftype p1; ModuleFunctionType p2; } u; \
        u.p2 = procaddr; \
        fptr = u.p1; }

static HANDLE OVR_Win32_SignCheck(FilePathCharType* fullPath)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WINTRUST_FILE_INFO fileData;
    WINTRUST_DATA wintrustData;
    GUID actionGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    LONG resultStatus;
    int verified = 0;
    HMODULE libWinTrust = LoadLibraryW(L"wintrust");
    HMODULE libCrypt32 = LoadLibraryW(L"crypt32");
    if (libWinTrust == NULL || libCrypt32 == NULL)
    {
        return INVALID_HANDLE_VALUE;
    }

    OVR_SIGNING_CONVERT_PTR(PtrCertGetNameStringW, m_PtrCertGetNameStringW, GetProcAddress(libCrypt32, "CertGetNameStringW"));
    OVR_SIGNING_CONVERT_PTR(PtrWinVerifyTrust, m_PtrWinVerifyTrust, GetProcAddress(libWinTrust, "WinVerifyTrust"));
    OVR_SIGNING_CONVERT_PTR(PtrWTHelperProvDataFromStateData, m_PtrWTHelperProvDataFromStateData, GetProcAddress(libWinTrust, "WTHelperProvDataFromStateData"));
    OVR_SIGNING_CONVERT_PTR(PtrWTHelperGetProvSignerFromChain, m_PtrWTHelperGetProvSignerFromChain, GetProcAddress(libWinTrust, "WTHelperGetProvSignerFromChain"));

    if (m_PtrCertGetNameStringW == NULL || m_PtrWinVerifyTrust == NULL ||
        m_PtrWTHelperProvDataFromStateData == NULL || m_PtrWTHelperGetProvSignerFromChain == NULL)
    {
        return INVALID_HANDLE_VALUE;
    }

    if (!fullPath)
    {
        return INVALID_HANDLE_VALUE;
    }

    hFile = CreateFileW(fullPath, GENERIC_READ, FILE_SHARE_READ,
                        0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    ZeroMemory(&fileData, sizeof(fileData));
    fileData.cbStruct      = sizeof(fileData);
    fileData.pcwszFilePath = fullPath;
    fileData.hFile         = hFile;

    ZeroMemory(&wintrustData, sizeof(wintrustData));
    wintrustData.cbStruct      = sizeof(wintrustData);
    wintrustData.pFile         = &fileData;
    wintrustData.dwUnionChoice = WTD_CHOICE_FILE; // Specify WINTRUST_FILE_INFO.
    wintrustData.dwUIChoice    = WTD_UI_NONE; // Do not display any UI.
    wintrustData.dwUIContext   = WTD_UICONTEXT_EXECUTE; // Hint that this is about app execution.
    wintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    wintrustData.dwProvFlags = WTD_REVOCATION_CHECK_NONE;
    wintrustData.dwStateAction = WTD_STATEACTION_VERIFY;
    wintrustData.hWVTStateData = 0;

    resultStatus = m_PtrWinVerifyTrust(
        (HWND)INVALID_HANDLE_VALUE, // Do not display any UI.
        &actionGUID, // V2 verification
        &wintrustData);

    if (resultStatus == ERROR_SUCCESS &&
        wintrustData.hWVTStateData != 0 &&
        wintrustData.hWVTStateData != INVALID_HANDLE_VALUE)
    {
        CRYPT_PROVIDER_DATA* cpd = m_PtrWTHelperProvDataFromStateData(wintrustData.hWVTStateData);
        if (cpd && cpd->csSigners == 1)
        {
            CRYPT_PROVIDER_SGNR* cps = m_PtrWTHelperGetProvSignerFromChain(cpd, 0, FALSE, 0);
            int chainIndex;
            for (chainIndex = 0; chainIndex < CertificateChainCount; ++chainIndex)
            {
                CertificateEntry* chain = AllowedCertificateChains[chainIndex];
                if (VCCRSuccess == ValidateCertificateContents(chain, cps))
                {
                    verified = 1;
                    break;
                }
            }
        }
    }

    wintrustData.dwStateAction = WTD_STATEACTION_CLOSE;

    m_PtrWinVerifyTrust(
        (HWND)INVALID_HANDLE_VALUE, // Do not display any UI.
        &actionGUID, // V2 verification
        &wintrustData);

    if (verified != 1)
    {
        CloseHandle(hFile);
        return INVALID_HANDLE_VALUE;
    }

    return hFile;
}

#endif // #if defined(_WIN32)

static ModuleHandleType OVR_OpenLibrary(const FilePathCharType* libraryPath, ovrResult* result)
{
    #if defined(_WIN32)
        DWORD fullPathNameLen = 0;
        FilePathCharType fullPath[MAX_PATH] = { 0 };
        HANDLE hFilePinned = INVALID_HANDLE_VALUE;
        ModuleHandleType hModule = 0;

        *result = ovrSuccess;
        fullPathNameLen = GetFullPathNameW(libraryPath, MAX_PATH, fullPath, 0);
        if (fullPathNameLen <= 0 || fullPathNameLen >= MAX_PATH)
        {
            *result = ovrError_LibPath;
            return NULL;
        }
        fullPath[MAX_PATH - 1] = 0;

        hFilePinned = OVR_Win32_SignCheck(fullPath);

            if (hFilePinned == INVALID_HANDLE_VALUE)
            {
                *result = ovrError_LibSignCheck;
                return NULL;
            }

        hModule = LoadLibraryW(fullPath);

        if (hFilePinned != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFilePinned);
        }

        return hModule;
    #else
        *result = ovrSuccess;

        // Don't bother trying to dlopen() a file that is not even there.
        if (access(libraryPath, X_OK | R_OK ) != 0)
        {
            *result = ovrError_LibPath;
            return NULL;
        }

        dlerror(); // Clear any previous dlopen() errors

        // Use RTLD_NOW because we don't want unexpected stalls at runtime, and the library isn't very large.
        // Use RTLD_LOCAL to avoid unilaterally exporting resolved symbols to the rest of this process.
        void *lib = dlopen(libraryPath, RTLD_NOW | RTLD_LOCAL);

        if (!lib)
        {
            #if defined(__APPLE__)
            // TODO: Output the error in whatever logging system OSX uses (jhughes)
            #else  // __APPLE__
            fprintf(stderr, "ERROR: Can't load '%s':\n%s\n", libraryPath, dlerror());
            #endif // __APPLE__
        }

        return lib;
    #endif
}


static void OVR_CloseLibrary(ModuleHandleType hLibrary)
{
    if (hLibrary)
    {
        #if defined(_WIN32)
            // We may need to consider what to do in the case that the library is in an exception state.
            // In a Windows C++ DLL, all global objects (including static members of classes) will be constructed just
            // before the calling of the DllMain with DLL_PROCESS_ATTACH and they will be destroyed just after
            // the call of the DllMain with DLL_PROCESS_DETACH. We may need to intercept DLL_PROCESS_DETACH and
            // have special handling for the case that the DLL is broken.
            FreeLibrary(hLibrary);
        #else
            dlclose(hLibrary);
        #endif
    }
}


// Returns a valid ModuleHandleType (e.g. Windows HMODULE) or returns ModuleHandleTypeNull (e.g. NULL).
// The caller is required to eventually call OVR_CloseLibrary on a valid return handle.
//
static ModuleHandleType OVR_FindLibraryPath(int requestedProductVersion, int requestedMajorVersion,
                               FilePathCharType* libraryPath, size_t libraryPathCapacity, ovrResult* result)
{
    ModuleHandleType moduleHandle;
    int printfResult;
    FilePathCharType developerDir[OVR_MAX_PATH] = { '\0' };

    #if defined(_MSC_VER)
        #if defined(_WIN64)
            const char* pBitDepth = "64";
        #else
            const char* pBitDepth = "32";
        #endif
    #elif defined(__APPLE__)
        // For Apple platforms we are using a Universal Binary LibOVRRT dylib which has both 32 and 64 in it.
    #else // Other Unix.
        #if defined(__x86_64__)
            const char* pBitDepth = "64";
        #else
            const char* pBitDepth = "32";
        #endif
    #endif

    (void)requestedProductVersion;

    *result = ovrError_LibLoad;
    moduleHandle = ModuleHandleTypeNull;
    if(libraryPathCapacity)
        libraryPath[0] = '\0';

    // Note: OVR_ENABLE_DEVELOPER_SEARCH is deprecated in favor of the simpler LIBOVR_DLL_DIR, as the edge
    // case uses of the former created some complications that may be best solved by simply using a LIBOVR_DLL_DIR
    // environment variable which the user can set in their debugger or system environment variables.
    #if (defined(_MSC_VER) || defined(_WIN32)) && !defined(OVR_FILE_PATH_SEPARATOR)
        #define OVR_FILE_PATH_SEPARATOR "\\"
    #else
        #define OVR_FILE_PATH_SEPARATOR "/"
    #endif

    {
        const char* pLibOvrDllDir = getenv("LIBOVR_DLL_DIR"); // Example value: /dev/OculusSDK/Main/LibOVR/Mac/Debug/

        if(pLibOvrDllDir)
        {
            char developerDir8[OVR_MAX_PATH];
            size_t length = OVR_strlcpy(developerDir8, pLibOvrDllDir, sizeof(developerDir8)); // If missing a trailing path separator then append one.

            if((length > 0) && (length < sizeof(developerDir8)) && (developerDir8[length - 1] != OVR_FILE_PATH_SEPARATOR[0]))
            {
                length = OVR_strlcat(developerDir8, OVR_FILE_PATH_SEPARATOR, sizeof(developerDir8));

                if(length < sizeof(developerDir8))
                {
                    #if defined(_WIN32)
                        size_t i;
                        for(i = 0; i <= length; ++i) // ASCII conversion of 8 to 16 bit text.
                            developerDir[i] = (FilePathCharType)(uint8_t)developerDir8[i];
                    #else
                        OVR_strlcpy(developerDir, developerDir8, sizeof(developerDir));
                    #endif
                }
            }
        }
    }

    // Support checking for a developer library location override via the OVR_SDK_ROOT environment variable.
    // This pathway is deprecated in favor of using LIBOVR_DLL_DIR instead.
    #if defined(OVR_ENABLE_DEVELOPER_SEARCH)
    if (!developerDir[0]) // If not already set by LIBOVR_DLL_PATH...
    {
        // __FILE__ maps to <sdkRoot>/LibOVR/Src/OVR_CAPIShim.c
        char sdkRoot[OVR_MAX_PATH];
        char* pLibOVR;
        size_t i;

        // We assume that __FILE__ returns a full path, which isn't the case for some compilers.
        // Need to compile with /FC under VC++ for __FILE__ to expand to the full file path.
        // clang expands __FILE__ to a full path by default.
        OVR_strlcpy(sdkRoot, __FILE__, sizeof(sdkRoot));
        for(i = 0; sdkRoot[i]; ++i)
            sdkRoot[i] = (char)tolower(sdkRoot[i]); // Microsoft doesn't maintain case.
        pLibOVR = strstr(sdkRoot, "libovr");
        if(pLibOVR && (pLibOVR > sdkRoot))
            pLibOVR[-1] = '\0';
        else
            sdkRoot[0] = '\0';

        if(sdkRoot[0])
        {
            // We want to use a developer version of the library only if the application is also being executed from
            // a developer location. Ideally we would do this by checking that the relative path from the executable to
            // the shared library is the same at runtime as it was when the executable was first built, but we don't have
            // an easy way to do that from here and it would require some runtime help from the application code.
            // Instead we verify that the application is simply in the same developer tree that was was when built.
            // We could put in some additional logic to make it very likely to know if the EXE is in its original location.
            FilePathCharType modulePath[OVR_MAX_PATH];
            const ovrBool pathMatch = OVR_GetCurrentModuleDirectory(modulePath, OVR_MAX_PATH, ovrTrue) &&
                                        (OVR_PathStartsWith(modulePath, sdkRoot) == ovrTrue);
            if(pathMatch == ovrFalse)
            {
                sdkRoot[0] = '\0'; // The application module is not in the developer tree, so don't try to use the developer shared library.
            }
        }

        if(sdkRoot[0])
        {
            #if defined(OVR_BUILD_DEBUG)
                const char* pConfigDirName = "Debug";
            #else
                const char* pConfigDirName = "Release";
            #endif

            #if defined(_MSC_VER)
                #if defined(_WIN64)
                    const char* pArchDirName = "x64";
                #else
                    const char* pArchDirName = "Win32";
                #endif
            #else
                #if defined(__x86_64__)
                    const char* pArchDirName = "x86_64";
                #else
                    const char* pArchDirName = "i386";
                #endif
            #endif

            #if defined(_MSC_VER) && (_MSC_VER == 1600)
                const char* pCompilerVersion = "VS2010";
            #elif defined(_MSC_VER) && (_MSC_VER == 1700)
                const char* pCompilerVersion = "VS2012";
            #elif defined(_MSC_VER) && (_MSC_VER == 1800)
                const char* pCompilerVersion = "VS2013";
            #elif defined(_MSC_VER) && (_MSC_VER == 1900)
                const char* pCompilerVersion = "VS2015";
            #endif

            #if defined(_WIN32)
                int count = swprintf_s(developerDir, OVR_MAX_PATH, L"%hs\\LibOVR\\Lib\\Windows\\%hs\\%hs\\%hs\\",
                                        sdkRoot, pArchDirName, pConfigDirName, pCompilerVersion);
            #elif defined(__APPLE__)
                // Apple/XCode doesn't let you specify an arch in build paths, which is OK if we build a universal binary.
                (void)pArchDirName;
                int count = snprintf(developerDir, OVR_MAX_PATH, "%s/LibOVR/Lib/Mac/%s/",
                                        sdkRoot, pConfigDirName);
            #else
                int count = snprintf(developerDir, OVR_MAX_PATH, "%s/LibOVR/Lib/Linux/%s/%s/",
                                        sdkRoot, pArchDirName, pConfigDirName);
            #endif

            if((count < 0) || (count >= (int)OVR_MAX_PATH)) // If there was an error or capacity overflow... clear the string.
            {
                developerDir[0] = '\0';
            }
        }
    }
    #endif // OVR_ENABLE_DEVELOPER_SEARCH

    {
        #if !defined(_WIN32)
            FilePathCharType cwDir[OVR_MAX_PATH]; // Will be filled in below.
            FilePathCharType appDir[OVR_MAX_PATH];
        #endif
        size_t i;

        #if defined(_WIN32)
            // On Windows, only search the developer directory and the usual path
            const FilePathCharType* directoryArray[2];
            directoryArray[0] = developerDir; // Developer directory.
            directoryArray[1] = L""; // No directory, which causes Windows to use the standard search strategy to find the DLL.

        #elif defined(__APPLE__)
            // https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man1/dyld.1.html

            FilePathCharType  homeDir[OVR_MAX_PATH];
            FilePathCharType  homeFrameworkDir[OVR_MAX_PATH];
            const FilePathCharType* directoryArray[5];
            size_t            homeDirLength = 0;

            const char* pHome = getenv("HOME"); // Try getting the HOME environment variable.

            if (pHome)
            {
                homeDirLength = OVR_strlcpy(homeDir, pHome, sizeof(homeDir));
            }
            else
            {
                // https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/getpwuid_r.3.html
                const long pwBufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);

                if (pwBufferSize != -1)
                {
                    char pwBuffer[pwBufferSize];
                    struct passwd  pw;
                    struct passwd* pwResult = NULL;

                    if ((getpwuid_r(getuid(), &pw, pwBuffer, pwBufferSize, &pwResult) == 0) && pwResult)
                        homeDirLength = OVR_strlcpy(homeDir, pw.pw_dir, sizeof(homeDir));
                }
            }

            if (homeDirLength)
            {
                if (homeDir[homeDirLength - 1] == '/')
                    homeDir[homeDirLength - 1] = '\0';
                OVR_strlcpy(homeFrameworkDir, homeDir, sizeof(homeFrameworkDir));
                OVR_strlcat(homeFrameworkDir, "/Library/Frameworks/", sizeof(homeFrameworkDir));
            }
            else
            {
                homeFrameworkDir[0] = '\0';
            }

            directoryArray[0] = cwDir;
            directoryArray[1] = appDir;
            directoryArray[2] = homeFrameworkDir;           // ~/Library/Frameworks/
            directoryArray[3] = "/Library/Frameworks/";     // DYLD_FALLBACK_FRAMEWORK_PATH
            directoryArray[4] = developerDir;               // Developer directory.

        #else
            #define STR1(x) #x
            #define STR(x)  STR1(x)
            #ifdef LIBDIR
                #define TEST_LIB_DIR STR(LIBDIR) "/"
            #else
                #define TEST_LIB_DIR appDir
            #endif

            const FilePathCharType* directoryArray[5];
            directoryArray[0] = cwDir;
            directoryArray[1] = TEST_LIB_DIR;           // Directory specified by LIBDIR if defined.
            directoryArray[2] = developerDir;           // Developer directory.
            directoryArray[3] = "/usr/local/lib/";
            directoryArray[4] = "/usr/lib/";
        #endif

        #if !defined(_WIN32)
            OVR_GetCurrentWorkingDirectory(cwDir, sizeof(cwDir) / sizeof(cwDir[0]));
            OVR_GetCurrentApplicationDirectory(appDir, sizeof(appDir) / sizeof(appDir[0]), ovrTrue, NULL);
        #endif

        // Versioned file expectations.
        //     Windows: LibOVRRT<BIT_DEPTH>_<PRODUCT_VERSION>_<MAJOR_VERSION>.dll                                  // Example: LibOVRRT64_1_1.dll -- LibOVRRT 64 bit, product 1, major version 1, minor/patch/build numbers unspecified in the name.
        //     Mac:     LibOVRRT_<PRODUCT_VERSION>.framework/Versions/<MAJOR_VERSION>/LibOVRRT_<PRODUCT_VERSION>   // We are not presently using the .framework bundle's Current directory to hold the version number. This may change.
        //     Linux:   libOVRRT<BIT_DEPTH>_<PRODUCT_VERSION>.so.<MAJOR_VERSION>                                   // The file on disk may contain a minor version number, but a symlink is used to map this major-only version to it.

        // Since we are manually loading the LibOVR dynamic library, we need to look in various locations for a file
        // that matches our requirements. The functionality required is somewhat similar to the operating system's
        // dynamic loader functionality. Each OS has some differences in how this is handled.
        // Future versions of this may iterate over all libOVRRT.so.* files in the directory and use the one that matches our requirements.
        //
        // We need to look for a library that matches the product version and major version of the caller's request,
        // and that library needs to support a minor version that is >= the requested minor version. Currently we
        // don't test the minor version here, as the library is named based only on the product and major version.
        // Currently the minor version test is handled via the initialization of the library and the initialization
        // fails if minor version cannot be supported by the library. The reason this is done during initialization
        // is that the library can at runtime support multiple minor versions based on the user's request. To the
        // external user, all that matters it that they call ovr_Initialize with a requested version and it succeeds
        // or fails.
        //
        // The product version is something that is at a higher level than the major version, and is not something that's
        // always seen in libraries (an example is the well-known LibXml2 library, in which the 2 is essentially the product version).

        for(i = 0; i < sizeof(directoryArray)/sizeof(directoryArray[0]); ++i)
        {
            #if defined(_WIN32)
                printfResult = swprintf(libraryPath, libraryPathCapacity, L"%lsLibOVRRT%hs_%d.dll", directoryArray[i], pBitDepth, requestedMajorVersion);

                if (*directoryArray[i] == 0)
                {
                    int k;
                    FilePathCharType foundPath[MAX_PATH] = { 0 };
                    DWORD searchResult = SearchPathW(NULL, libraryPath, NULL, MAX_PATH, foundPath, NULL);
                    if (searchResult <= 0 || searchResult >= libraryPathCapacity)
                    {
                        continue;
                    }
                    foundPath[MAX_PATH - 1] = 0;
                    for (k = 0; k < MAX_PATH; ++k)
                    {
                        libraryPath[k] = foundPath[k];
                    }
                }

            #elif defined(__APPLE__)
                // https://developer.apple.com/library/mac/documentation/MacOSX/Conceptual/BPFrameworks/Concepts/VersionInformation.html
                // Macintosh application bundles have the option of embedding dependent frameworks within the application
                // bundle itself. A problem with that is that it doesn't support vendor-supplied updates to the framework.
                printfResult = snprintf(libraryPath, libraryPathCapacity, "%sLibOVRRT.framework/Versions/%d/LibOVRRT", directoryArray[i], requestedMajorVersion);

            #else // Unix
                // Applications that depend on the OS (e.g. ld-linux / ldd) can rely on the library being in a common location
                // such as /usr/lib or can rely on the -rpath linker option to embed a path for the OS to check for the library,
                // or can rely on the LD_LIBRARY_PATH environment variable being set. It's generally not recommended that applications
                // depend on LD_LIBRARY_PATH be globally modified, partly due to potentialy security issues.
                // Currently we check the current application directory, current working directory, and then /usr/lib and possibly others.
                printfResult = snprintf(libraryPath, libraryPathCapacity, "%slibOVRRT%s.so.%d", directoryArray[i], pBitDepth, requestedMajorVersion);
            #endif

            if((printfResult >= 0) && (printfResult < (int)libraryPathCapacity))
            {
                moduleHandle = OVR_OpenLibrary(libraryPath, result);
                if(moduleHandle != ModuleHandleTypeNull)
                    return moduleHandle;
            }
        }
    }

    return moduleHandle;
}



//-----------------------------------------------------------------------------------
// ***** hLibOVR
//
// global handle to the LivOVR shared library.
//
static ModuleHandleType hLibOVR = NULL;

// This function is currently unsupported.
ModuleHandleType ovr_GetLibOVRRTHandle()
{
    return hLibOVR;
}



//-----------------------------------------------------------------------------------
// ***** Function declarations
//

//-----------------------------------------------------------------------------------
// ***** OVR_DECLARE_IMPORT
//
// Creates a pointer and loader value union for each entry in OVR_LIST_APIS()
//

#define OVR_DECLARE_IMPORT(ReturnValue, FunctionName, OptionalVersion, Arguments) \
    union { \
        ReturnValue (OVR_CDECL* Ptr) Arguments; \
        ModuleFunctionType Symbol; \
    } FunctionName;

#define OVR_IGNORE_IMPORT(ReturnValue, FunctionName, OptionalVersion, Arguments)

//-----------------------------------------------------------------------------------
// ***** API - a structure with each API entrypoint as a FunctionName.Ptr and FunctionName.Symbol union
//

static struct
{
    OVR_LIST_APIS(OVR_DECLARE_IMPORT, OVR_IGNORE_IMPORT)
} API = { NULL };

static void OVR_UnloadSharedLibrary()
{
    memset(&API, 0, sizeof(API));
    if (hLibOVR)
        OVR_CloseLibrary(hLibOVR);
    hLibOVR = NULL;
}

static ovrResult OVR_LoadSharedLibrary(int requestedProductVersion, int requestedMajorVersion)
{
    FilePathCharType filePath[OVR_MAX_PATH];
    const char *SymbolName = NULL;
    ovrResult result = ovrSuccess;

    if (hLibOVR)
        return result;

    hLibOVR = OVR_FindLibraryPath(requestedProductVersion, requestedMajorVersion, filePath, sizeof(filePath) / sizeof(filePath[0]), &result);

    if (!hLibOVR)
        return result;

    // Zero the API table just to be paranoid
    memset(&API, 0, sizeof(API));

    // Load the current API entrypoint using the catenated FunctionName and OptionalVersion
    #define OVR_GETFUNCTION(ReturnValue, FunctionName, OptionalVersion, Arguments) \
        SymbolName = #FunctionName #OptionalVersion; \
        API.FunctionName.Symbol = OVR_DLSYM(hLibOVR, SymbolName); \
        if (!API.FunctionName.Symbol) \
        {\
            result = ovrError_LibSymbols; \
            goto FailedToLoadSymbol; \
        }

    OVR_LIST_APIS(OVR_GETFUNCTION, OVR_IGNORE_IMPORT)

    #undef OVR_GETFUNCTION

    return result;

FailedToLoadSymbol:
    // Check SymbolName for the name of the API which failed to load
    OVR_UnloadSharedLibrary();
    return result;
}

// These defaults are also in CAPI.cpp
static const ovrInitParams DefaultParams = {
    ovrInit_RequestVersion, // Flags
    OVR_MINOR_VERSION,      // RequestedMinorVersion
    0,                      // LogCallback
    0,                      // UserData
    0,                      // ConnectionTimeoutSeconds
    OVR_ON64("")            // pad0
};

OVR_PUBLIC_FUNCTION(ovrResult) ovr_Initialize(const ovrInitParams* inputParams)
{
    ovrResult result;
    ovrInitParams params;

    typedef void (OVR_CDECL *ovr_ReportClientInfoType)(
        unsigned int compilerVersion,
        int productVersion, int majorVersion, int minorVersion,
        int patchVersion, int buildNumber);
    ovr_ReportClientInfoType reportClientInfo;

    // Do something with our version signature hash to prevent
    // it from being optimized out. In this case, compute
    // a cheap CRC.
    uint8_t crc = 0;
    size_t i;

    for (i = 0; i < (sizeof(OculusSDKUniqueIdentifier) - 3); ++i) // Minus 3 because we have trailing OVR_MAJOR_VERSION, OVR_MINOR_VERSION, OVR_PATCH_VERSION which vary per version.
    {
        crc ^= OculusSDKUniqueIdentifier[i];
    }

    assert(crc == OculusSDKUniqueIdentifierXORResult);
    if (crc != OculusSDKUniqueIdentifierXORResult)
    {
        return ovrError_Initialize;
    }

    if (!inputParams)
    {
        params = DefaultParams;
    }
    else
    {
        params = *inputParams;

        // If not requesting a particular minor version,
        if (!(params.Flags & ovrInit_RequestVersion))
        {
            // Enable requesting the default minor version.
            params.Flags |= ovrInit_RequestVersion;
            params.RequestedMinorVersion = OVR_MINOR_VERSION;
        }
    }

    // Clear non-writable bits provided by client code.
    params.Flags &= ovrinit_WritableBits;



    // By design we ignore the build version in the library search.
    result = OVR_LoadSharedLibrary(OVR_PRODUCT_VERSION, OVR_MAJOR_VERSION);
    if (result != ovrSuccess)
       return result;

    result = API.ovr_Initialize.Ptr(&params);
    if (result != ovrSuccess)
        OVR_UnloadSharedLibrary();

    reportClientInfo = (ovr_ReportClientInfoType)(uintptr_t)OVR_DLSYM(hLibOVR, "ovr_ReportClientInfo");

    if (reportClientInfo)
    {
        unsigned int mscFullVer = 0;
#if defined (_MSC_FULL_VER)
        mscFullVer = _MSC_FULL_VER;
#endif // _MSC_FULL_VER

        reportClientInfo(mscFullVer, OVR_PRODUCT_VERSION, OVR_MAJOR_VERSION,
            OVR_MINOR_VERSION, OVR_PATCH_VERSION, OVR_BUILD_NUMBER);
    }

    return result;
}

OVR_PUBLIC_FUNCTION(void) ovr_Shutdown()
{
    if (!API.ovr_Shutdown.Ptr)
        return;
    API.ovr_Shutdown.Ptr();
    OVR_UnloadSharedLibrary();
}

OVR_PUBLIC_FUNCTION(const char*) ovr_GetVersionString()
{
    // We don't directly return the value of the DLL API.ovr_GetVersionString.Ptr call,
    // because that call returns a pointer to memory within the DLL. If the DLL goes
    // away then that pointer becomes invalid while the process may still be holding
    // onto it. So we save a local copy of it which is always valid.
    static char dllVersionStringLocal[32];
    const char* dllVersionString;

    if (!API.ovr_GetVersionString.Ptr)
        return "(Unable to load LibOVR)";

    dllVersionString = API.ovr_GetVersionString.Ptr(); // Guaranteed to always be valid.
    assert(dllVersionString != NULL);
    OVR_strlcpy(dllVersionStringLocal, dllVersionString, sizeof(dllVersionStringLocal));

    return dllVersionStringLocal;
}

OVR_PUBLIC_FUNCTION(void) ovr_GetLastErrorInfo(ovrErrorInfo* errorInfo)
{
    if (!API.ovr_GetLastErrorInfo.Ptr)
    {
        memset(errorInfo, 0, sizeof(ovrErrorInfo));
        errorInfo->Result = ovrError_NotInitialized;
    }
    else
        API.ovr_GetLastErrorInfo.Ptr(errorInfo);
}

OVR_PUBLIC_FUNCTION(ovrHmdDesc) ovr_GetHmdDesc(ovrSession session)
{
    if (!API.ovr_GetHmdDesc.Ptr)
    {
        ovrHmdDesc hmdDesc;
        memset(&hmdDesc, 0, sizeof(hmdDesc));
        hmdDesc.Type = ovrHmd_None;
        return hmdDesc;
    }

    return API.ovr_GetHmdDesc.Ptr(session);
}

OVR_PUBLIC_FUNCTION(unsigned int) ovr_GetTrackerCount(ovrSession session)
{
    if (!API.ovr_GetTrackerCount.Ptr)
    {
        return 0;
    }

    return API.ovr_GetTrackerCount.Ptr(session);
}

OVR_PUBLIC_FUNCTION(ovrTrackerDesc) ovr_GetTrackerDesc(ovrSession session, unsigned int trackerDescIndex)
{
    if (!API.ovr_GetTrackerDesc.Ptr)
    {
        ovrTrackerDesc trackerDesc;
        memset(&trackerDesc, 0, sizeof(trackerDesc));
        return trackerDesc;
    }

    return API.ovr_GetTrackerDesc.Ptr(session, trackerDescIndex);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_Create(ovrSession* pSession, ovrGraphicsLuid* pLuid)
{
    if (!API.ovr_Create.Ptr)
        return ovrError_NotInitialized;
    return API.ovr_Create.Ptr(pSession, pLuid);
}

OVR_PUBLIC_FUNCTION(void) ovr_Destroy(ovrSession session)
{
    if (!API.ovr_Destroy.Ptr)
        return;
    API.ovr_Destroy.Ptr(session);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetSessionStatus(ovrSession session, ovrSessionStatus* sessionStatus)
{
    if (!API.ovr_GetSessionStatus.Ptr)
    {
        if (sessionStatus)
        {
            sessionStatus->IsVisible   = ovrFalse;
            sessionStatus->HmdPresent  = ovrFalse;
            sessionStatus->HmdMounted  = ovrFalse;
            sessionStatus->ShouldQuit  = ovrFalse;
            sessionStatus->DisplayLost = ovrFalse;
            sessionStatus->ShouldRecenter = ovrFalse;
        }

        return ovrError_NotInitialized;
    }

    return API.ovr_GetSessionStatus.Ptr(session, sessionStatus);
}


OVR_PUBLIC_FUNCTION(ovrResult) ovr_SetTrackingOriginType(ovrSession session, ovrTrackingOrigin origin)
{
    if (!API.ovr_SetTrackingOriginType.Ptr)
        return ovrError_NotInitialized;
    return API.ovr_SetTrackingOriginType.Ptr(session, origin);
}

OVR_PUBLIC_FUNCTION(ovrTrackingOrigin) ovr_GetTrackingOriginType(ovrSession session)
{
    if (!API.ovr_GetTrackingOriginType.Ptr)
        return ovrTrackingOrigin_EyeLevel;
    return API.ovr_GetTrackingOriginType.Ptr(session);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_RecenterTrackingOrigin(ovrSession session)
{
    if (!API.ovr_RecenterTrackingOrigin.Ptr)
        return ovrError_NotInitialized;
    return API.ovr_RecenterTrackingOrigin.Ptr(session);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_SpecifyTrackingOrigin(ovrSession session, ovrPosef originPose)
{
    if (!API.ovr_SpecifyTrackingOrigin.Ptr)
        return ovrError_NotInitialized;
    return API.ovr_SpecifyTrackingOrigin.Ptr(session, originPose);
}

OVR_PUBLIC_FUNCTION(void) ovr_ClearShouldRecenterFlag(ovrSession session)
{
    if (!API.ovr_ClearShouldRecenterFlag.Ptr)
        return;
    API.ovr_ClearShouldRecenterFlag.Ptr(session);
}

OVR_PUBLIC_FUNCTION(ovrTrackingState) ovr_GetTrackingState(ovrSession session, double absTime, ovrBool latencyMarker)
{
    if (!API.ovr_GetTrackingState.Ptr)
    {
        ovrTrackingState nullTrackingState;
        memset(&nullTrackingState, 0, sizeof(nullTrackingState));
        return nullTrackingState;
    }

    return API.ovr_GetTrackingState.Ptr(session, absTime, latencyMarker);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetDevicePoses(ovrSession session, ovrTrackedDeviceType* deviceTypes, int deviceCount, double absTime, ovrPoseStatef* outDevicePoses)
{
    if (!API.ovr_GetDevicePoses.Ptr)
        return ovrError_NotInitialized;
    return API.ovr_GetDevicePoses.Ptr(session, deviceTypes, deviceCount, absTime, outDevicePoses);
}

OVR_PUBLIC_FUNCTION(ovrTrackingState) ovr_GetTrackingStateWithSensorData(ovrSession session, double absTime, ovrBool latencyMarker, ovrSensorData* sensorData)
{
    if (!API.ovr_GetTrackingStateWithSensorData.Ptr)
    {
        ovrTrackingState nullTrackingState;
        memset(&nullTrackingState, 0, sizeof(nullTrackingState));
        if (sensorData)
            memset(&sensorData, 0, sizeof(sensorData));
        return nullTrackingState;
    }

    return API.ovr_GetTrackingStateWithSensorData.Ptr(session, absTime, latencyMarker, sensorData);
}

OVR_PUBLIC_FUNCTION(ovrTrackerPose) ovr_GetTrackerPose(ovrSession session, unsigned int trackerPoseIndex)
{
    if (!API.ovr_GetTrackerPose.Ptr)
    {
        ovrTrackerPose nullTrackerPose;
        memset(&nullTrackerPose, 0, sizeof(nullTrackerPose));
        return nullTrackerPose;
    }

    return API.ovr_GetTrackerPose.Ptr(session, trackerPoseIndex);
}


OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetInputState(ovrSession session, ovrControllerType controllerType, ovrInputState* inputState)
{
    if (!API.ovr_GetInputState.Ptr)
    {
        if (inputState)
            memset(inputState, 0, sizeof(ovrInputState));
        return ovrError_NotInitialized;
    }
    return API.ovr_GetInputState.Ptr(session, controllerType, inputState);
}

OVR_PUBLIC_FUNCTION(unsigned int) ovr_GetConnectedControllerTypes(ovrSession session)
{
    if (!API.ovr_GetConnectedControllerTypes.Ptr)
    {
        return 0;
    }
    return API.ovr_GetConnectedControllerTypes.Ptr(session);
}

OVR_PUBLIC_FUNCTION(ovrTouchHapticsDesc) ovr_GetTouchHapticsDesc(ovrSession session, ovrControllerType controllerType)
{
    if (!API.ovr_GetTouchHapticsDesc.Ptr)
    {
        ovrTouchHapticsDesc nullDesc;
        memset(&nullDesc, 0, sizeof(nullDesc));
        return nullDesc;
    }

    return API.ovr_GetTouchHapticsDesc.Ptr(session, controllerType);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_SetControllerVibration(ovrSession session, ovrControllerType controllerType, float frequency, float amplitude)
{
    if (!API.ovr_SetControllerVibration.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_SetControllerVibration.Ptr(session, controllerType, frequency, amplitude);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_SubmitControllerVibration(ovrSession session, ovrControllerType controllerType, const ovrHapticsBuffer* buffer)
{
    if (!API.ovr_SubmitControllerVibration.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_SubmitControllerVibration.Ptr(session, controllerType, buffer);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetControllerVibrationState(ovrSession session, ovrControllerType controllerType, ovrHapticsPlaybackState* outState)
{
    if (!API.ovr_GetControllerVibrationState.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetControllerVibrationState.Ptr(session, controllerType, outState);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_TestBoundary(ovrSession session, ovrTrackedDeviceType deviceBitmask, ovrBoundaryType singleBoundaryType, ovrBoundaryTestResult* outTestResult)
{
    if (!API.ovr_TestBoundary.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_TestBoundary.Ptr(session, deviceBitmask, singleBoundaryType, outTestResult);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_TestBoundaryPoint(ovrSession session, const ovrVector3f* point, ovrBoundaryType singleBoundaryType, ovrBoundaryTestResult* outTestResult)
{
    if (!API.ovr_TestBoundaryPoint.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_TestBoundaryPoint.Ptr(session, point, singleBoundaryType, outTestResult);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_SetBoundaryLookAndFeel(ovrSession session, const ovrBoundaryLookAndFeel* lookAndFeel)
{
    if (!API.ovr_SetBoundaryLookAndFeel.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_SetBoundaryLookAndFeel.Ptr(session, lookAndFeel);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_ResetBoundaryLookAndFeel(ovrSession session)
{
    if (!API.ovr_ResetBoundaryLookAndFeel.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_ResetBoundaryLookAndFeel.Ptr(session);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetBoundaryGeometry(ovrSession session, ovrBoundaryType singleBoundaryType, ovrVector3f* outFloorPoints, int* outFloorPointsCount)
{
    if (!API.ovr_GetBoundaryGeometry.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetBoundaryGeometry.Ptr(session, singleBoundaryType, outFloorPoints, outFloorPointsCount);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetBoundaryDimensions(ovrSession session, ovrBoundaryType singleBoundaryType, ovrVector3f* outDimensions)
{
    if (!API.ovr_GetBoundaryDimensions.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetBoundaryDimensions.Ptr(session, singleBoundaryType, outDimensions);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetBoundaryVisible(ovrSession session, ovrBool* outIsVisible)
{
    if (!API.ovr_GetBoundaryVisible.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetBoundaryVisible.Ptr(session, outIsVisible);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_RequestBoundaryVisible(ovrSession session, ovrBool visible)
{
    if (!API.ovr_RequestBoundaryVisible.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_RequestBoundaryVisible.Ptr(session, visible);
}

OVR_PUBLIC_FUNCTION(ovrSizei) ovr_GetFovTextureSize(ovrSession session, ovrEyeType eye, ovrFovPort fov,
                                             float pixelsPerDisplayPixel)
{
    if (!API.ovr_GetFovTextureSize.Ptr)
    {
        ovrSizei nullSize;
        memset(&nullSize, 0, sizeof(nullSize));
        return nullSize;
    }

    return API.ovr_GetFovTextureSize.Ptr(session, eye, fov, pixelsPerDisplayPixel);
}

#if defined (_WIN32)
OVR_PUBLIC_FUNCTION(ovrResult) ovr_CreateTextureSwapChainDX(ovrSession session,
                                                            IUnknown* d3dPtr,
                                                            const ovrTextureSwapChainDesc* desc,
                                                            ovrTextureSwapChain* outTextureSet)
{
    if (!API.ovr_CreateTextureSwapChainDX.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_CreateTextureSwapChainDX.Ptr(session, d3dPtr, desc, outTextureSet);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CreateMirrorTextureDX(ovrSession session,
                                                         IUnknown* d3dPtr,
                                                         const ovrMirrorTextureDesc* desc,
                                                         ovrMirrorTexture* outMirrorTexture)
{
    if (!API.ovr_CreateMirrorTextureDX.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_CreateMirrorTextureDX.Ptr(session, d3dPtr, desc, outMirrorTexture);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainBufferDX(ovrSession session,
                                                               ovrTextureSwapChain chain,
                                                               int index,
                                                               IID iid,
                                                               void** ppObject)
{
    if (!API.ovr_GetTextureSwapChainBufferDX.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetTextureSwapChainBufferDX.Ptr(session, chain, index, iid, ppObject);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetMirrorTextureBufferDX(ovrSession session,
                                                            ovrMirrorTexture mirror,
                                                            IID iid,
                                                            void** ppObject)
{
    if (!API.ovr_GetMirrorTextureBufferDX.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetMirrorTextureBufferDX.Ptr(session, mirror, iid, ppObject);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceOutWaveId(unsigned int* deviceOutId)
{
    if (!API.ovr_GetAudioDeviceOutWaveId.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetAudioDeviceOutWaveId.Ptr(deviceOutId);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceInWaveId(unsigned int* deviceInId)
{
    if (!API.ovr_GetAudioDeviceInWaveId.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetAudioDeviceInWaveId.Ptr(deviceInId);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceOutGuidStr(WCHAR* deviceOutStrBuffer)
{
    if (!API.ovr_GetAudioDeviceOutGuidStr.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetAudioDeviceOutGuidStr.Ptr(deviceOutStrBuffer);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceOutGuid(GUID* deviceOutGuid)
{
    if (!API.ovr_GetAudioDeviceOutGuid.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetAudioDeviceOutGuid.Ptr(deviceOutGuid);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceInGuidStr(WCHAR* deviceInStrBuffer)
{
    if (!API.ovr_GetAudioDeviceInGuidStr.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetAudioDeviceInGuidStr.Ptr(deviceInStrBuffer);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceInGuid(GUID* deviceInGuid)
{
    if (!API.ovr_GetAudioDeviceInGuid.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetAudioDeviceInGuid.Ptr(deviceInGuid);
}

#endif

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CreateTextureSwapChainGL(ovrSession session,
                                                            const ovrTextureSwapChainDesc* desc,
                                                            ovrTextureSwapChain* outTextureSet)
{
    if (!API.ovr_CreateTextureSwapChainGL.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_CreateTextureSwapChainGL.Ptr(session, desc, outTextureSet);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CreateMirrorTextureGL(ovrSession session,
                                                         const ovrMirrorTextureDesc* desc,
                                                         ovrMirrorTexture* outMirrorTexture)
{
    if (!API.ovr_CreateMirrorTextureGL.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_CreateMirrorTextureGL.Ptr(session, desc, outMirrorTexture);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainBufferGL(ovrSession session,
                                                               ovrTextureSwapChain chain,
                                                               int index,
                                                               unsigned int* texId)
{
    if (!API.ovr_GetTextureSwapChainBufferGL.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetTextureSwapChainBufferGL.Ptr(session, chain, index, texId);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetMirrorTextureBufferGL(ovrSession session,
                                                            ovrMirrorTexture mirror,
                                                            unsigned int* texId)
{
    if (!API.ovr_GetMirrorTextureBufferGL.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetMirrorTextureBufferGL.Ptr(session, mirror, texId);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainLength(ovrSession session,
                                                             ovrTextureSwapChain chain,
                                                             int* length)
{
    if (!API.ovr_GetTextureSwapChainLength.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetTextureSwapChainLength.Ptr(session, chain, length);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainCurrentIndex(ovrSession session,
                                                                   ovrTextureSwapChain chain,
                                                                   int* currentIndex)
{
    if (!API.ovr_GetTextureSwapChainCurrentIndex.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetTextureSwapChainCurrentIndex.Ptr(session, chain, currentIndex);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainDesc(ovrSession session,
                                                           ovrTextureSwapChain chain,
                                                           ovrTextureSwapChainDesc* desc)
{
    if (!API.ovr_GetTextureSwapChainDesc.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetTextureSwapChainDesc.Ptr(session, chain, desc);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CommitTextureSwapChain(ovrSession session,
                                                          ovrTextureSwapChain chain)
{
    if (!API.ovr_CommitTextureSwapChain.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_CommitTextureSwapChain.Ptr(session, chain);
}

OVR_PUBLIC_FUNCTION(void) ovr_DestroyTextureSwapChain(ovrSession session, ovrTextureSwapChain chain)
{
    if (!API.ovr_DestroyTextureSwapChain.Ptr)
        return;

    API.ovr_DestroyTextureSwapChain.Ptr(session, chain);
}

OVR_PUBLIC_FUNCTION(void) ovr_DestroyMirrorTexture(ovrSession session, ovrMirrorTexture mirrorTexture)
{
    if (!API.ovr_DestroyMirrorTexture.Ptr)
        return;

    API.ovr_DestroyMirrorTexture.Ptr(session, mirrorTexture);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_SubmitFrame(ovrSession session, long long frameIndex, const ovrViewScaleDesc* viewScaleDesc, ovrLayerHeader const * const * layerPtrList, unsigned int layerCount)
{
    if (!API.ovr_SubmitFrame.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_SubmitFrame.Ptr(session, frameIndex, viewScaleDesc, layerPtrList, layerCount);
}

OVR_PUBLIC_FUNCTION(ovrEyeRenderDesc) ovr_GetRenderDesc(ovrSession session, ovrEyeType eyeType, ovrFovPort fov)
{
    if (!API.ovr_GetRenderDesc.Ptr)
    {
        ovrEyeRenderDesc nullEyeRenderDesc;
        memset(&nullEyeRenderDesc, 0, sizeof(nullEyeRenderDesc));
        return nullEyeRenderDesc;
    }
    return API.ovr_GetRenderDesc.Ptr(session, eyeType, fov);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetPerfStats(ovrSession session, ovrPerfStats* outPerfStats)
{
    if (!API.ovr_GetPerfStats.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_GetPerfStats.Ptr(session, outPerfStats);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_ResetPerfStats(ovrSession session)
{
    if (!API.ovr_ResetPerfStats.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_ResetPerfStats.Ptr(session);
}

OVR_PUBLIC_FUNCTION(double) ovr_GetPredictedDisplayTime(ovrSession session, long long frameIndex)
{
    if (!API.ovr_GetPredictedDisplayTime.Ptr)
        return 0.0;

    return API.ovr_GetPredictedDisplayTime.Ptr(session, frameIndex);
}

OVR_PUBLIC_FUNCTION(double) ovr_GetTimeInSeconds()
{
    if (!API.ovr_GetTimeInSeconds.Ptr)
        return 0.;
    return API.ovr_GetTimeInSeconds.Ptr();
}

OVR_PUBLIC_FUNCTION(ovrBool) ovr_GetBool(ovrSession session, const char* propertyName, ovrBool defaultVal)
{
    if (!API.ovr_GetBool.Ptr)
        return ovrFalse;
    return API.ovr_GetBool.Ptr(session, propertyName, defaultVal);
}

OVR_PUBLIC_FUNCTION(ovrBool) ovr_SetBool(ovrSession session, const char* propertyName, ovrBool value)
{
    if (!API.ovr_SetBool.Ptr)
        return ovrFalse;
    return API.ovr_SetBool.Ptr(session, propertyName, value);
}

OVR_PUBLIC_FUNCTION(int) ovr_GetInt(ovrSession session, const char* propertyName, int defaultVal)
{
    if (!API.ovr_GetInt.Ptr)
        return 0;
    return API.ovr_GetInt.Ptr(session, propertyName, defaultVal);
}

OVR_PUBLIC_FUNCTION(ovrBool) ovr_SetInt(ovrSession session, const char* propertyName, int value)
{
    if (!API.ovr_SetInt.Ptr)
        return ovrFalse;
    return API.ovr_SetInt.Ptr(session, propertyName, value);
}

OVR_PUBLIC_FUNCTION(float) ovr_GetFloat(ovrSession session, const char* propertyName, float defaultVal)
{
    if (!API.ovr_GetFloat.Ptr)
        return 0.f;
    return API.ovr_GetFloat.Ptr(session, propertyName, defaultVal);
}

OVR_PUBLIC_FUNCTION(ovrBool) ovr_SetFloat(ovrSession session, const char* propertyName, float value)
{
    if (!API.ovr_SetFloat.Ptr)
        return ovrFalse;
    return API.ovr_SetFloat.Ptr(session, propertyName, value);
}

OVR_PUBLIC_FUNCTION(unsigned int) ovr_GetFloatArray(ovrSession session, const char* propertyName,
                                            float values[], unsigned int arraySize)
{
    if (!API.ovr_GetFloatArray.Ptr)
        return 0;
    return API.ovr_GetFloatArray.Ptr(session, propertyName, values, arraySize);
}

OVR_PUBLIC_FUNCTION(ovrBool) ovr_SetFloatArray(ovrSession session, const char* propertyName,
                                             const float values[], unsigned int arraySize)
{
    if (!API.ovr_SetFloatArray.Ptr)
        return ovrFalse;
    return API.ovr_SetFloatArray.Ptr(session, propertyName, values, arraySize);
}

OVR_PUBLIC_FUNCTION(const char*) ovr_GetString(ovrSession session, const char* propertyName,
                                        const char* defaultVal)
{
    if (!API.ovr_GetString.Ptr)
        return "(Unable to load LibOVR)";
    return API.ovr_GetString.Ptr(session, propertyName, defaultVal);
}

OVR_PUBLIC_FUNCTION(ovrBool) ovr_SetString(ovrSession session, const char* propertyName,
                                    const char* value)
{
    if (!API.ovr_SetString.Ptr)
        return ovrFalse;
    return API.ovr_SetString.Ptr(session, propertyName, value);
}

OVR_PUBLIC_FUNCTION(int) ovr_TraceMessage(int level, const char* message)
{
    if (!API.ovr_TraceMessage.Ptr)
        return -1;

    return API.ovr_TraceMessage.Ptr(level, message);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_IdentifyClient(const char* identity)
{
    if (!API.ovr_IdentifyClient.Ptr)
        return ovrError_NotInitialized;

    return API.ovr_IdentifyClient.Ptr(identity);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_Lookup(const char* name, void** data)
{
    if (!API.ovr_Lookup.Ptr)
        return ovrError_NotInitialized;
    return API.ovr_Lookup.Ptr(name, data);
}

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

