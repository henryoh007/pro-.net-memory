#include "stdafx.h"
//#define HOSTINGCORECLR 1

#include <stdio.h>
#ifdef HOSTINGCORECLR
#include "mscoree.h"
#else
#include <metahost.h>
#include <mscoree.h>
#pragma comment(lib, "mscoree.lib")
#endif
#include <vector>
#include <string>
#include <iostream>
using namespace std;

#ifdef HOSTINGCORECLR
typedef enum {
    COR_GC_COUNTS = 0x00000001,
    COR_GC_MEMORYUSAGE = 0x00000002
} COR_GC_STAT_TYPES;
#endif

class CustomHostGCManager : public IHostGCManager
{
    ULONG referenceCounter;
public:
    // Inherited via IHostGCManager
    virtual HRESULT ThreadIsBlockingForSuspension(void) override
    {
        return S_OK;
    }
    virtual HRESULT SuspensionStarting(void) override
    {
        return S_OK;
    }
    virtual HRESULT SuspensionEnding(DWORD Generation) override
    {
        return S_OK;
    }
    // Inherited via IUnknown
    virtual ULONG AddRef(void) override
    {
        return referenceCounter++;
    }
    virtual ULONG Release(void) override
    {
        return referenceCounter--;
    }
    virtual HRESULT QueryInterface(REFIID riid, void ** ppvObject) override
    {
        if (!ppvObject) return E_POINTER;
        *ppvObject = this;
        return S_OK;
    }
};

///////////////////////////////////////////////////////////////////////
// Listing 15-25
class CustomHostMalloc : public IHostMalloc
{
    ULONG referenceCounter;

public:
    CustomHostMalloc() : referenceCounter(0) { }

    // Inherited via IHostMalloc
    virtual HRESULT Alloc(SIZE_T cbSize, EMemoryCriticalLevel eCriticalLevel, void ** ppMem) override
    {
        *ppMem = ::malloc(cbSize);
        cout << "   Alloc " << *ppMem << " (" << cbSize << ")" << endl;
        return S_OK;
    }
    virtual HRESULT DebugAlloc(SIZE_T cbSize, EMemoryCriticalLevel eCriticalLevel, char * pszFileName, int iLineNo, void ** ppMem) override
    {
        *ppMem = ::malloc(cbSize);
        return S_OK;
    }
    virtual HRESULT Free(void * pMem) override
    {
        ::free(pMem);
        return S_OK;
    }

    // Inherited via IUnknown
    virtual ULONG AddRef(void) override
    {
        return referenceCounter++;
    }
    virtual ULONG Release(void) override
    {
        return referenceCounter--;
    }
    virtual HRESULT QueryInterface(REFIID riid, void ** ppvObject) override
    {
        if (!ppvObject) return E_POINTER;
        *ppvObject = this;
        return S_OK;
    }
};

///////////////////////////////////////////////////////////////////////
// Listing 15-24
class CustomHostMemoryManager : public IHostMemoryManager
{
    ULONG referenceCounter;

public:
    CustomHostMemoryManager() : referenceCounter(0) { }

    // Inherited via IHostMemoryManager
    virtual HRESULT CreateMalloc(DWORD dwMallocType, IHostMalloc ** ppMalloc) override
    {
        *ppMalloc = new CustomHostMalloc();
        return S_OK;
    }
    virtual HRESULT VirtualAlloc(void * pAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect, EMemoryCriticalLevel eCriticalLevel, void ** ppMem) override
    {
         void* result = ::VirtualAlloc(pAddress, dwSize, flAllocationType, flProtect);
         *ppMem = result;
        BOOL locked = false;
        if (flAllocationType & MEM_COMMIT)
        {
            //locked = ::VirtualLock(*ppMem, dwSize);
        }
        cout << "VirtualAlloc " << *ppMem << " (" << dwSize << "), flags: " << flAllocationType << " " << flProtect << " => " << pAddress << " " << locked << endl;
        return S_OK;
    }
    virtual HRESULT VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType) override
    {
        ::VirtualFree(lpAddress, dwSize, dwFreeType);
        return S_OK;
    }
    virtual HRESULT VirtualQuery(void * lpAddress, void * lpBuffer, SIZE_T dwLength, SIZE_T * pResult) override
    {
        *pResult = ::VirtualQuery(lpAddress, (PMEMORY_BASIC_INFORMATION)lpBuffer, dwLength);
        return S_OK;
    }
    virtual HRESULT VirtualProtect(void * lpAddress, SIZE_T dwSize, DWORD flNewProtect, DWORD * pflOldProtect) override
    {
        ::VirtualProtect(lpAddress, dwSize, flNewProtect, pflOldProtect);
        return S_OK;
    }
    virtual HRESULT GetMemoryLoad(DWORD * pMemoryLoad, SIZE_T * pAvailableBytes) override
    {
        *pMemoryLoad = 10; 
        *pAvailableBytes = 1024 * 1024 * 1024;
        return S_OK;
    }
    virtual HRESULT RegisterMemoryNotificationCallback(ICLRMemoryNotificationCallback * pCallback) override
    {
        return S_OK;
    }
    virtual HRESULT NeedsVirtualAddressSpace(LPVOID startAddress, SIZE_T size) override
    {
        return S_OK;
    }
    virtual HRESULT AcquiredVirtualAddressSpace(LPVOID startAddress, SIZE_T size) override
    {
        return S_OK;
    }
    virtual HRESULT ReleasedVirtualAddressSpace(LPVOID startAddress) override
    {
        return S_OK;
    }

    // Inherited via IUnknown
    virtual ULONG AddRef(void) override
    {
        return referenceCounter++;
    }
    virtual ULONG Release(void) override
    {
        return referenceCounter--;
    }
    virtual HRESULT QueryInterface(REFIID riid, void ** ppvObject) override
    {
        if (!ppvObject) return E_POINTER;
        *ppvObject = this;
        return S_OK;
    }
};

///////////////////////////////////////////////////////////////////////
// Listing 15-23
class CustomHostControl : public IHostControl
{
    ULONG referenceCounter;
    
public:
    CustomHostControl()
    {
        referenceCounter = 0;
    }

    // Inherited via IHostControl
    virtual HRESULT GetHostManager(REFIID riid, void ** ppObject) override
    {
        if (riid == IID_IHostMemoryManager)
        {
            IHostMemoryManager *pMemoryManager = new CustomHostMemoryManager();
            *ppObject = pMemoryManager;
            return S_OK;
        }
        *ppObject = NULL;
        return E_NOINTERFACE;
    }
    virtual HRESULT QueryInterface(const IID &riid, void **ppvObject)
    {
        if (riid == IID_IUnknown)
        {
            *ppvObject = static_cast<IUnknown*>(static_cast<IHostControl*>(this));
            return S_OK;
        }
        if (riid == IID_IHostControl)
        {
            *ppvObject = static_cast<IHostControl*>(this);
            return S_OK;
        }
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    virtual HRESULT SetAppDomainManager(DWORD dwAppDomainID, IUnknown * pUnkAppDomainManager) override
    {
        return S_OK;
    }
    virtual ULONG AddRef()
    {
        return referenceCounter++;
    }
    virtual ULONG Release()
    {
        return referenceCounter--;
    }
};




int wmain(int argc, wchar_t* argv[])
{
    cout << "Starting..." << endl;

    HRESULT hr;
    ICLRRuntimeHost* runtimeHost;

    const wchar_t *AppPath =
#ifdef HOSTINGCORECLR
        L"F:\\GithubProjects\\NetMemoryBook\\Projects\\CoreCLR.General\\CoreCLR.HelloWorld\\bin\\x64\\Debug\\netcoreapp2.0\\CoreCLR.HelloWorld.dll";
#else
        L"F:\\GithubProjects\\NetMemoryBook\\Projects\\CoreCLR.General\\DotNet.HelloWorld\\bin\\Release\\DotNet.HelloWorld.exe";
#endif
    wchar_t targetApp[MAX_PATH];
    GetFullPathNameW(AppPath, MAX_PATH, targetApp, NULL);
    //GetFullPathNameW(argv[1], MAX_PATH, targetApp, NULL);
    wchar_t targetAppPath[MAX_PATH];
    wcscpy_s(targetAppPath, targetApp);
    size_t i = wcslen(targetAppPath - 1);
    while (i >= 0 && targetAppPath[i] != L'\\') i--;
    targetAppPath[i] = L'\0';

#ifdef HOSTINGCORECLR
    const wchar_t *CoreCLRPath = L"C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\2.1.0";
    const wchar_t *CoreCLRFile = L"C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\2.1.0\\coreclr.dll";
    vector <wstring> args(argv, argv + argc);
    // The managed application to run should be the first command-line parameter.
    // Subsequent command line parameters will be passed to the managed app later in this host.

    // Load coreclr.dll
    HMODULE coreclrHandle = LoadLibraryExW(CoreCLRFile, NULL, 0);

    // Listing 15-18
    ICLRRuntimeHost4* runtimeHost4;
    FnGetCLRRuntimeHost pfnGetCLRRuntimeHost = (FnGetCLRRuntimeHost)::GetProcAddress(coreclrHandle, "GetCLRRuntimeHost");
    if (!pfnGetCLRRuntimeHost)
    {
        printf("ERROR - GetCLRRuntimeHost not found");
        return -1;
    }
    // Get the hosting interface
    hr = pfnGetCLRRuntimeHost(IID_ICLRRuntimeHost4, (IUnknown**)&runtimeHost4);
    
    hr = runtimeHost4->SetStartupFlags(
        static_cast<STARTUP_FLAGS>(
            // STARTUP_FLAGS::STARTUP_SERVER_GC |								// Use server GC
            // STARTUP_FLAGS::STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN |		// Maximize domain-neutral loading
            // STARTUP_FLAGS::STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST |	// Domain-neutral loading for strongly-named assemblies
            STARTUP_FLAGS::STARTUP_CONCURRENT_GC |						// Use concurrent GC
            STARTUP_FLAGS::STARTUP_SINGLE_APPDOMAIN |					// All code executes in the default AppDomain 
                                                                        // (required to use the runtimeHost->ExecuteAssembly helper function)
            STARTUP_FLAGS::STARTUP_LOADER_OPTIMIZATION_SINGLE_DOMAIN	// Prevents domain-neutral loading
            )
    );
    runtimeHost = runtimeHost4;
#else
    ICLRMetaHost    *pMetaHost = nullptr;
    ICLRRuntimeInfo *pRuntimeInfo = nullptr;

    hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&pMetaHost);
    hr = pMetaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&pRuntimeInfo));
    hr = pRuntimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_ICLRRuntimeHost, (LPVOID*)&runtimeHost);
#endif

    ///////////////////////////////////////////////////////////////////////////
    ICLRControl* clrControl;
    hr = runtimeHost->GetCLRControl(&clrControl);
    
    // Listing 15-22
    CustomHostControl customHostControl;
    hr = runtimeHost->SetHostControl(&customHostControl);

    // Listing 15-20
    ICLRGCManager2* clrGCManager;
    hr = clrControl->GetCLRManager(IID_ICLRGCManager2, (void**)&clrGCManager);
    SIZE_T segmentSize = 4 * 1024 * 1024 * 1024;
    SIZE_T maxGen0Size = 4 * 1024 * 1024 * 1024;
    hr = clrGCManager->SetGCStartupLimitsEx(segmentSize, maxGen0Size);

    // Getting this interface enables AppDomain Resource Monitoring
    ICLRAppDomainResourceMonitor *monitor;
    hr = clrControl->GetCLRManager(IID_ICLRAppDomainResourceMonitor, (void**)&monitor);

    hr = runtimeHost->Start();

    ///////////////////////////////////////////////////////////////////////////


#ifdef HOSTINGCORECLR
    int appDomainFlags =
        // APPDOMAIN_FORCE_TRIVIAL_WAIT_OPERATIONS |		// Do not pump messages during wait
        // APPDOMAIN_SECURITY_SANDBOXED |					// Causes assemblies not from the TPA list to be loaded as partially trusted
        APPDOMAIN_ENABLE_PLATFORM_SPECIFIC_APPS |			// Enable platform-specific assemblies to run
        APPDOMAIN_ENABLE_PINVOKE_AND_CLASSIC_COMINTEROP |	// Allow PInvoking from non-TPA assemblies
        APPDOMAIN_DISABLE_TRANSPARENCY_ENFORCEMENT;			// Entirely disables transparency checks 

    DWORD domainId;

    // TRUSTED_PLATFORM_ASSEMBLIES
    // "Trusted Platform Assemblies" are prioritized by the loader and always loaded with full trust.
    // A common pattern is to include any assemblies next to CoreCLR.dll as platform assemblies.
    // More sophisticated hosts may also include their own Framework extensions (such as AppDomain managers)
    // in this list.
    int tpaSize = 100 * MAX_PATH; // Starting size for our TPA (Trusted Platform Assemblies) list
    wchar_t* trustedPlatformAssemblies = new wchar_t[tpaSize];
    trustedPlatformAssemblies[0] = L'\0';

    // Extensions to probe for when finding TPA list files
    wchar_t *tpaExtensions[] = {
        L"*.dll",
        L"*.exe",
        L"*.winmd"
    };

    // Probe next to CoreCLR.dll for any files matching the extensions from tpaExtensions and
    // add them to the TPA list. In a real host, this would likely be extracted into a separate function
    // and perhaps also run on other directories of interest.
    for (int i = 0; i < _countof(tpaExtensions); i++)
    {
        // Construct the file name search pattern
        wchar_t searchPath[MAX_PATH];
        wcscpy_s(searchPath, MAX_PATH, CoreCLRPath);
        wcscat_s(searchPath, MAX_PATH, L"\\");
        wcscat_s(searchPath, MAX_PATH, tpaExtensions[i]);

        // Find files matching the search pattern
        WIN32_FIND_DATAW findData;
        HANDLE fileHandle = FindFirstFileW(searchPath, &findData);

        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                // Construct the full path of the trusted assembly
                wchar_t pathToAdd[MAX_PATH];
                wcscpy_s(pathToAdd, MAX_PATH, CoreCLRPath);
                wcscat_s(pathToAdd, MAX_PATH, L"\\");
                wcscat_s(pathToAdd, MAX_PATH, findData.cFileName);

                // Check to see if TPA list needs expanded
                if (wcslen(pathToAdd) + (3) + wcslen(trustedPlatformAssemblies) >= tpaSize)
                {
                    // Expand, if needed
                    tpaSize *= 2;
                    wchar_t* newTPAList = new wchar_t[tpaSize];
                    wcscpy_s(newTPAList, tpaSize, trustedPlatformAssemblies);
                    trustedPlatformAssemblies = newTPAList;
                }

                // Add the assembly to the list and delimited with a semi-colon
                wcscat_s(trustedPlatformAssemblies, tpaSize, pathToAdd);
                wcscat_s(trustedPlatformAssemblies, tpaSize, L";");

                // Note that the CLR does not guarantee which assembly will be loaded if an assembly
                // is in the TPA list multiple times (perhaps from different paths or perhaps with different NI/NI.dll
                // extensions. Therefore, a real host should probably add items to the list in priority order and only
                // add a file if it's not already present on the list.
                //
                // For this simple sample, though, and because we're only loading TPA assemblies from a single path,
                // we can ignore that complication.
            } while (FindNextFileW(fileHandle, &findData));
            FindClose(fileHandle);
        }
    }

    // Setup key/value pairs for AppDomain  properties
    const wchar_t* propertyKeys[] = {
        L"TRUSTED_PLATFORM_ASSEMBLIES",
        L"APP_PATHS",
        L"APP_NI_PATHS",
        L"NATIVE_DLL_SEARCH_DIRECTORIES",
        L"PLATFORM_RESOURCE_ROOTS",
        L"AppDomainCompatSwitch"
    };
    const wchar_t* propertyValues[] = {
        trustedPlatformAssemblies,
        targetAppPath,
        targetAppPath,
        targetAppPath,
        targetAppPath,
        L"UseLatestBehaviorWhenTFMNotSpecified"
    };

    // Create the AppDomain
    hr = runtimeHost4->CreateAppDomainWithManager(
        L"Sample Host AppDomain",		// Friendly AD name
        appDomainFlags,
        NULL,							// Optional AppDomain manager assembly name
        NULL,							// Optional AppDomain manager type (including namespace)
        sizeof(propertyKeys) / sizeof(wchar_t*),
        propertyKeys,
        propertyValues,
        &domainId);

    DWORD exitCode = -1;
    hr = runtimeHost4->ExecuteAssembly(domainId, targetApp, argc - 1, (LPCWSTR*)(argc > 1 ? &argv[1] : NULL), &exitCode);
#else
    DWORD dwReturn;
    hr = runtimeHost->ExecuteInDefaultAppDomain(targetApp, L"DotNet.HelloWorld.Program", L"Test", L"", &dwReturn);
#endif

    DWORD appDomainId;
    unsigned long long allocated;
    unsigned long long survivedAppDomain;
    unsigned long long survivedTotal;
    hr = runtimeHost->GetCurrentAppDomainId(&appDomainId);
    hr = monitor->GetCurrentAllocated(appDomainId, &allocated);
    hr = monitor->GetCurrentSurvived(appDomainId, &survivedAppDomain, &survivedTotal);

    // Listing 15-21
    _COR_GC_STATS gcStats;
    gcStats.Flags = COR_GC_COUNTS | COR_GC_MEMORYUSAGE;
    // Based on perf counters so does not work in CoreCLR 
    hr = clrGCManager->GetStats(&gcStats);
    cout << gcStats.CommittedKBytes << endl
        << gcStats.Gen0HeapSizeKBytes << endl
        << gcStats.Gen1HeapSizeKBytes << endl
        << gcStats.Gen2HeapSizeKBytes << endl
        << gcStats.LargeObjectHeapSizeKBytes << endl
        << gcStats.ExplicitGCCount << endl
        << gcStats.GenCollectionsTaken[0] << endl
        << gcStats.GenCollectionsTaken[1] << endl
        << gcStats.GenCollectionsTaken[2] << endl;        


#ifdef HOSTINGCORECLR
    runtimeHost->UnloadAppDomain(domainId, true /* Wait until unload complete */);
#endif
    runtimeHost->Stop();
    runtimeHost->Release();
    return 0;
}

