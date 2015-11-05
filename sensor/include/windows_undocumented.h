/*
Copyright 2015 refractionPOINT

Licensed under the Apache License, Version 2.0 ( the "License" );
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _UNDOCUMENTED_H
#define _UNDOCUMENTED_H
#include <Windows.h>
#pragma warning( disable: 4214 )
#include <WinDNS.h>
#include <DbgHelp.h>
#include <wincrypt.h>

//
// Most of those have been taken from REACTOS
//

#pragma warning( disable: 4200 ) // Disabling error on zero-sized arrays
#pragma warning( disable: 4201 ) // Disabling error on nameless struct or union
#pragma warning( disable: 4115 ) // Disabling error on named type definition in parentheses 

// In some cases, SAL are not defined.
// If this is the case, define it here.
#ifndef __struct_bcount
#define __struct_bcount(size)
#endif
#ifndef __field_range
#define __field_range(op,size)
#endif
#ifndef __field_ecount_opt
#define __field_ecount_opt(count)
#endif


//
// internet types
//

typedef LPVOID HINTERNET;
typedef HINTERNET * LPHINTERNET;

typedef WORD INTERNET_PORT;
typedef INTERNET_PORT * LPINTERNET_PORT;


//
// INTERNET_SCHEME - enumerated URL scheme type
//

typedef enum {
    INTERNET_SCHEME_PARTIAL = -2,
    INTERNET_SCHEME_UNKNOWN = -1,
    INTERNET_SCHEME_DEFAULT = 0,
    INTERNET_SCHEME_FTP,
    INTERNET_SCHEME_GOPHER,
    INTERNET_SCHEME_HTTP,
    INTERNET_SCHEME_HTTPS,
    INTERNET_SCHEME_FILE,
    INTERNET_SCHEME_NEWS,
    INTERNET_SCHEME_MAILTO,
    INTERNET_SCHEME_SOCKS,
    INTERNET_SCHEME_JAVASCRIPT,
    INTERNET_SCHEME_VBSCRIPT,
    INTERNET_SCHEME_RES,
    INTERNET_SCHEME_FIRST = INTERNET_SCHEME_FTP,
    INTERNET_SCHEME_LAST = INTERNET_SCHEME_RES
} INTERNET_SCHEME, * LPINTERNET_SCHEME;

typedef struct {
    DWORD   dwStructSize;       // size of this structure. Used in version check
    LPSTR   lpszScheme;         // pointer to scheme name
    DWORD   dwSchemeLength;     // length of scheme name
    INTERNET_SCHEME nScheme;    // enumerated scheme type (if known)
    LPSTR   lpszHostName;       // pointer to host name
    DWORD   dwHostNameLength;   // length of host name
    INTERNET_PORT nPort;        // converted port number
    LPSTR   lpszUserName;       // pointer to user name
    DWORD   dwUserNameLength;   // length of user name
    LPSTR   lpszPassword;       // pointer to password
    DWORD   dwPasswordLength;   // length of password
    LPSTR   lpszUrlPath;        // pointer to URL-path
    DWORD   dwUrlPathLength;    // length of URL-path
    LPSTR   lpszExtraInfo;      // pointer to extra information (e.g. ?foo or #foo)
    DWORD   dwExtraInfoLength;  // length of extra information
} URL_COMPONENTSA, * LPURL_COMPONENTSA;
typedef struct {
    DWORD   dwStructSize;       // size of this structure. Used in version check
    LPWSTR  lpszScheme;         // pointer to scheme name
    DWORD   dwSchemeLength;     // length of scheme name
    INTERNET_SCHEME nScheme;    // enumerated scheme type (if known)
    LPWSTR  lpszHostName;       // pointer to host name
    DWORD   dwHostNameLength;   // length of host name
    INTERNET_PORT nPort;        // converted port number
    LPWSTR  lpszUserName;       // pointer to user name
    DWORD   dwUserNameLength;   // length of user name
    LPWSTR  lpszPassword;       // pointer to password
    DWORD   dwPasswordLength;   // length of password
    LPWSTR  lpszUrlPath;        // pointer to URL-path
    DWORD   dwUrlPathLength;    // length of URL-path
    LPWSTR  lpszExtraInfo;      // pointer to extra information (e.g. ?foo or #foo)
    DWORD   dwExtraInfoLength;  // length of extra information
} URL_COMPONENTSW, * LPURL_COMPONENTSW;


typedef LONG 	NTSTATUS;
typedef LONG * 	PNTSTATUS;
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)

typedef struct _CLIENT_ID {
  HANDLE  UniqueProcess;
  HANDLE  UniqueThread;
} CLIENT_ID, *PCLIENT_ID;


typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PEB_LDR_DATA {
    ULONG          Length;
    BOOLEAN        Initialized;
    HANDLE         SsHandle;
    LIST_ENTRY     LoadOrder;
    LIST_ENTRY     MemoryOrder;
    LIST_ENTRY     InitializationOrder;
} PEB_LDR_DATA, *PPEB_LDR_DATA;


typedef struct _PEB_FREE_BLOCK {
    struct _PEB_FREE_BLOCK *Next;
    ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

#define GDI_HANDLE_BUFFER_SIZE          34
#define TLS_MINIMUM_AVAILABLE           64    // winnt
#define WIN32_CLIENT_INFO_LENGTH        31
#define WIN32_CLIENT_INFO_SPIN_COUNT    1
#define STATIC_UNICODE_BUFFER_LENGTH    261


typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct RTL_DRIVE_LETTER_CURDIR
{
    USHORT              Flags;
    USHORT              Length;
    ULONG               TimeStamp;
    UNICODE_STRING      DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;


typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    HANDLE              ConsoleHandle;
    ULONG               ConsoleFlags;
    HANDLE              hStdInput;
    HANDLE              hStdOutput;
    HANDLE              hStdError;
    CURDIR              CurrentDirectory;
    UNICODE_STRING      DllPath;
    UNICODE_STRING      ImagePathName;
    UNICODE_STRING      CommandLine;
    PWSTR               Environment;
    ULONG               dwX;
    ULONG               dwY;
    ULONG               dwXSize;
    ULONG               dwYSize;
    ULONG               dwXCountChars;
    ULONG               dwYCountChars;
    ULONG               dwFillAttribute;
    ULONG               dwFlags;
    ULONG               wShowWindow;
    UNICODE_STRING      WindowTitle;
    UNICODE_STRING      Desktop;
    UNICODE_STRING      ShellInfo;
    UNICODE_STRING      RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

/*
typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE Reserved1[16];
	PVOID Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;
*/

typedef struct tagRTL_BITMAP {
    ULONG  SizeOfBitMap; /* Number of bits in the bitmap */
    PULONG Buffer; /* Bitmap data, assumed sized to a DWORD boundary */
} RTL_BITMAP, *PRTL_BITMAP;

/*
typedef struct _PEB {
    BOOLEAN InheritedAddressSpace;      // These four fields cannot change unless the
    BOOLEAN ReadImageFileExecOptions;   //
    BOOLEAN BeingDebugged;              //
    BOOLEAN SpareBool;                  //
    HANDLE Mutant;                      // INITIAL_PEB structure is also updated.

    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
    PVOID FastPebLock;
    PVOID FastPebLockRoutine;
    PVOID FastPebUnlockRoutine;
    ULONG EnvironmentUpdateCount;
    PVOID KernelCallbackTable;
    HANDLE EventLogSection;
    PVOID EventLog;
    PPEB_FREE_BLOCK FreeList;
    ULONG TlsExpansionCounter;
    PVOID TlsBitmap;
    ULONG TlsBitmapBits[2];         // relates to TLS_MINIMUM_AVAILABLE
    PVOID ReadOnlySharedMemoryBase;
    PVOID ReadOnlySharedMemoryHeap;
    PVOID *ReadOnlyStaticServerData;
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCaseTableData;

    // Useful information for LdrpInitialize
    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;

    // Passed up from MmCreatePeb from Session Manager registry key

    LARGE_INTEGER CriticalSectionTimeout;
    ULONG HeapSegmentReserve;
    ULONG HeapSegmentCommit;
    ULONG HeapDeCommitTotalFreeThreshold;
    ULONG HeapDeCommitFreeBlockThreshold;

    // Where heap manager keeps track of all heaps created for a process
    // Fields initialized by MmCreatePeb.  ProcessHeaps is initialized
    // to point to the first free byte after the PEB and MaximumNumberOfHeaps
    // is computed from the page size used to hold the PEB, less the fixed
    // size of this data structure.

    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    PVOID *ProcessHeaps;

    //
    //
    PVOID GdiSharedHandleTable;
    PVOID ProcessStarterHelper;
    PVOID GdiDCAttributeList;
    PVOID LoaderLock;

    // Following fields filled in by MmCreatePeb from system values and/or
    // image header.

    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    ULONG OSBuildNumber;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    ULONG ImageProcessAffinityMask;
    ULONG GdiHandleBuffer[GDI_HANDLE_BUFFER_SIZE];
} PEB, *PPEB;
*/

/*
typedef ULONG PPS_POST_PROCESS_INIT_ROUTINE;
typedef struct _PEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	BYTE Reserved4[104];
	PVOID Reserved5[52];
	PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE Reserved6[128];
	PVOID Reserved7[1];
	ULONG SessionId;
} PEB, *PPEB;
*/

typedef struct _PEB
{
    BOOLEAN                      InheritedAddressSpace;             /*  00 */
    BOOLEAN                      ReadImageFileExecOptions;          /*  01 */
    BOOLEAN                      BeingDebugged;                     /*  02 */
    BOOLEAN                      SpareBool;                         /*  03 */
    HANDLE                       Mutant;                            /*  04 */
    HMODULE                      ImageBaseAddress;                  /*  08 */
    PPEB_LDR_DATA                LdrData;                           /*  0c */
    RTL_USER_PROCESS_PARAMETERS *ProcessParameters;                 /*  10 */
    PVOID                        SubSystemData;                     /*  14 */
    HANDLE                       ProcessHeap;                       /*  18 */
    PRTL_CRITICAL_SECTION        FastPebLock;                       /*  1c */
    PVOID /*PPEBLOCKROUTINE*/    FastPebLockRoutine;                /*  20 */
    PVOID /*PPEBLOCKROUTINE*/    FastPebUnlockRoutine;              /*  24 */
    ULONG                        EnvironmentUpdateCount;            /*  28 */
    PVOID                        KernelCallbackTable;               /*  2c */
    PVOID                        EventLogSection;                   /*  30 */
    PVOID                        EventLog;                          /*  34 */
    PVOID /*PPEB_FREE_BLOCK*/    FreeList;                          /*  38 */
    ULONG                        TlsExpansionCounter;               /*  3c */
    PRTL_BITMAP                  TlsBitmap;                         /*  40 */
    ULONG                        TlsBitmapBits[2];                  /*  44 */
    PVOID                        ReadOnlySharedMemoryBase;          /*  4c */
    PVOID                        ReadOnlySharedMemoryHeap;          /*  50 */
    PVOID                       *ReadOnlyStaticServerData;          /*  54 */
    PVOID                        AnsiCodePageData;                  /*  58 */
    PVOID                        OemCodePageData;                   /*  5c */
    PVOID                        UnicodeCaseTableData;              /*  60 */
    ULONG                        NumberOfProcessors;                /*  64 */
    ULONG                        NtGlobalFlag;                      /*  68 */
    BYTE                         Spare2[4];                         /*  6c */
    LARGE_INTEGER                CriticalSectionTimeout;            /*  70 */
    ULONG                        HeapSegmentReserve;                /*  78 */
    ULONG                        HeapSegmentCommit;                 /*  7c */
    ULONG                        HeapDeCommitTotalFreeThreshold;    /*  80 */
    ULONG                        HeapDeCommitFreeBlockThreshold;    /*  84 */
    ULONG                        NumberOfHeaps;                     /*  88 */
    ULONG                        MaximumNumberOfHeaps;              /*  8c */
    PVOID                       *ProcessHeaps;                      /*  90 */
    PVOID                        GdiSharedHandleTable;              /*  94 */
    PVOID                        ProcessStarterHelper;              /*  98 */
    PVOID                        GdiDCAttributeList;                /*  9c */
    PVOID                        LoaderLock;                        /*  a0 */
    ULONG                        OSMajorVersion;                    /*  a4 */
    ULONG                        OSMinorVersion;                    /*  a8 */
    ULONG                        OSBuildNumber;                     /*  ac */
    ULONG                        OSPlatformId;                      /*  b0 */
    ULONG                        ImageSubSystem;                    /*  b4 */
    ULONG                        ImageSubSystemMajorVersion;        /*  b8 */
    ULONG                        ImageSubSystemMinorVersion;        /*  bc */
    ULONG                        ImageProcessAffinityMask;          /*  c0 */
    ULONG                        GdiHandleBuffer[34];               /*  c4 */
    ULONG                        PostProcessInitRoutine;            /* 14c */
    PRTL_BITMAP                  TlsExpansionBitmap;                /* 150 */
    ULONG                        TlsExpansionBitmapBits[32];        /* 154 */
    ULONG                        SessionId;                         /* 1d4 */
} PEB, *PPEB;

typedef struct _PROCESS_BASIC_INFORMATION
{
    PVOID ExitStatus; // NTSTATUS
    PPEB PebBaseAddress;
    PVOID AffinityMask;
    PVOID BasePriority;
    ULONG_PTR UniqueProcessId;
    PVOID InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;

//
// Gdi command batching
//

#define GDI_BATCH_BUFFER_SIZE 310

typedef struct _GDI_TEB_BATCH {
    ULONG Offset;
    ULONG HDC;
    ULONG Buffer[GDI_BATCH_BUFFER_SIZE];
} GDI_TEB_BATCH,*PGDI_TEB_BATCH;




typedef struct _TEB {
    NT_TIB NtTib;
    PVOID  EnvironmentPointer;
    CLIENT_ID ClientId;
    PVOID ActiveRpcHandle;
    PVOID ThreadLocalStoragePointer;
    PPEB ProcessEnvironmentBlock;
    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    PVOID CsrClientThread;
    PVOID Win32ThreadInfo;          // PtiCurrent
    ULONG Win32ClientInfo[WIN32_CLIENT_INFO_LENGTH];    // User32 Client Info
    PVOID WOW32Reserved;           // used by WOW
    LCID CurrentLocale;
    ULONG FpSoftwareStatusRegister;
    PVOID SystemReserved1[54];      // Used by FP emulator
    PVOID Spare1;                   // unused
    NTSTATUS ExceptionCode;         // for RaiseUserException
    UCHAR SpareBytes1[40];
    PVOID SystemReserved2[10];                      // Used by user/console for temp obja
    GDI_TEB_BATCH GdiTebBatch;      // Gdi batching
    ULONG gdiRgn;
    ULONG gdiPen;
    ULONG gdiBrush;
    CLIENT_ID RealClientId;
    HANDLE GdiCachedProcessHandle;
    ULONG GdiClientPID;
    ULONG GdiClientTID;
    PVOID GdiThreadLocalInfo;
    PVOID UserReserved[5];          // unused
    PVOID glDispatchTable[280];     // OpenGL
    ULONG glReserved1[26];          // OpenGL
    PVOID glReserved2;              // OpenGL
    PVOID glSectionInfo;            // OpenGL
    PVOID glSection;                // OpenGL
    PVOID glTable;                  // OpenGL
    PVOID glCurrentRC;              // OpenGL
    PVOID glContext;                // OpenGL
    ULONG LastStatusValue;
    UNICODE_STRING StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];
    PVOID DeallocationStack;
    PVOID TlsSlots[TLS_MINIMUM_AVAILABLE];
    LIST_ENTRY TlsLinks;
    PVOID Vdm;
    PVOID ReservedForNtRpc;
    PVOID DbgSsReserved[2];
    ULONG HardErrorsAreDisabled;
    PVOID Instrumentation[16];
    PVOID WinSockData;              // WinSock
    ULONG GdiBatchCount;
    ULONG Spare2;
    ULONG Spare3;
    ULONG Spare4;
    PVOID ReservedForOle;
    ULONG WaitingOnLoaderLock;
} TEB;



typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,          // Note: this is kernel mode only
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    MaxProcessInfoClass
} PROCESSINFOCLASS;


typedef NTSTATUS (WINAPI *pfnNtQueryInformationProcess)(
    IN  HANDLE ProcessHandle,
    IN  PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN  ULONG ProcessInformationLength,
    OUT PULONG ReturnLength    OPTIONAL
    );


typedef enum _POOL_TYPE { 
  NonPagedPool,
  NonPagedPoolExecute                   = NonPagedPool,
  PagedPool,
  NonPagedPoolMustSucceed               = NonPagedPool + 2,
  DontUseThisType,
  NonPagedPoolCacheAligned              = NonPagedPool + 4,
  PagedPoolCacheAligned,
  NonPagedPoolCacheAlignedMustS         = NonPagedPool + 6,
  MaxPoolType,
  NonPagedPoolBase                      = 0,
  NonPagedPoolBaseMustSucceed           = NonPagedPoolBase + 2,
  NonPagedPoolBaseCacheAligned          = NonPagedPoolBase + 4,
  NonPagedPoolBaseCacheAlignedMustS     = NonPagedPoolBase + 6,
  NonPagedPoolSession                   = 32,
  PagedPoolSession                      = NonPagedPoolSession + 1,
  NonPagedPoolMustSucceedSession        = PagedPoolSession + 1,
  DontUseThisTypeSession                = NonPagedPoolMustSucceedSession + 1,
  NonPagedPoolCacheAlignedSession       = DontUseThisTypeSession + 1,
  PagedPoolCacheAlignedSession          = NonPagedPoolCacheAlignedSession + 1,
  NonPagedPoolCacheAlignedMustSSession  = PagedPoolCacheAlignedSession + 1,
  NonPagedPoolNx                        = 512,
  NonPagedPoolNxCacheAligned            = NonPagedPoolNx + 4,
  NonPagedPoolSessionNx                 = NonPagedPoolNx + 32
} POOL_TYPE;


typedef enum _OBJECT_INFORMATION_CLASS {
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllInformation,
    ObjectDataInformation
} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;


typedef struct _OBJECT_NAME_INFORMATION {
  UNICODE_STRING          Name;
  WCHAR                   NameBuffer[0];
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef struct _OBJECT_TYPE_INFORMATION {
  UNICODE_STRING          TypeName;
  ULONG                   TotalNumberOfHandles;
  ULONG                   TotalNumberOfObjects;
  WCHAR                   Unused1[8];
  ULONG                   HighWaterNumberOfHandles;
  ULONG                   HighWaterNumberOfObjects;
  WCHAR                   Unused2[8];
  ACCESS_MASK             InvalidAttributes;
  GENERIC_MAPPING         GenericMapping;
  ACCESS_MASK             ValidAttributes;
  BOOLEAN                 SecurityRequired;
  BOOLEAN                 MaintainHandleCount;
  USHORT                  MaintainTypeList;
  POOL_TYPE               PoolType;
  ULONG                   DefaultPagedPoolCharge;
  ULONG                   DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef NTSTATUS (WINAPI *pfnNtQueryObject)(
  IN HANDLE               ObjectHandle,
  IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
  OUT PVOID               ObjectInformation,
  IN ULONG                Length,
  OUT PULONG              ResultLength
  );


// Code to get handles in usermode through NtQuerySystemInformation
typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemNextEventIdInformation,
    SystemEventIdsInformation,
    SystemCrashDumpInformation,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,
    SystemPrioritySeperation,
    SystemPlugPlayBusInformation,
    SystemDockInformation,
    //SystemPowerInformation,
    SystemProcessorSpeedInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
	USHORT UniqueProcessId;
	USHORT CreatorBackTraceIndex;
	UCHAR ObjectTypeIndex;
	UCHAR HandleAttributes;
	USHORT HandleValue;
	PVOID Object;
	ULONG GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;


typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG NumberOfHandles;
	SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef NTSTATUS (WINAPI *pfnNtQuerySystemInformation)(
  SYSTEM_INFORMATION_CLASS SystemInformationClass,
  PVOID SystemInformation,
  ULONG SystemInformationLength,
  PULONG ReturnLength
);


typedef BOOL (WINAPI *pfnGetSystemTimes)(
  LPFILETIME lpIdleTime,
  LPFILETIME lpKernelTime,
  LPFILETIME lpUserTime
);


typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation=1,
    FileFullDirectoryInformation,
    FileBothDirectoryInformation,
    FileBasicInformation,
    FileStandardInformation,
    FileInternalInformation,
    FileEaInformation,
    FileAccessInformation,
    FileNameInformation,
    FileRenameInformation,
    FileLinkInformation,
    FileNamesInformation,
    FileDispositionInformation,
    FilePositionInformation,
    FileFullEaInformation,
    FileModeInformation,
    FileAlignmentInformation,
    FileAllInformation,
    FileAllocationInformation,
    FileEndOfFileInformation,
    FileAlternateNameInformation,
    FileStreamInformation,
    FilePipeInformation,
    FilePipeLocalInformation,
    FilePipeRemoteInformation,
    FileMailslotQueryInformation,
    FileMailslotSetInformation,
    FileCompressionInformation,
    FileCopyOnWriteInformation,
    FileCompletionInformation,
    FileMoveClusterInformation,
    FileQuotaInformation,
    FileReparsePointInformation,
    FileNetworkOpenInformation,
    FileObjectIdInformation,
    FileTrackingInformation,
    FileOleDirectoryInformation,
    FileContentIndexInformation,
    FileInheritContentIndexInformation,
    FileOleInformation,
    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID    Pointer;
  };
  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _FILE_NAME_INFORMATION {
  ULONG                   FileNameLength;
  WCHAR                   FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION {
  ULONG                   NextEntryOffset;
  ULONG                   FileIndex;
  ULONG                   FileNameLength;
  WCHAR                   FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef NTSTATUS (WINAPI *pfnNtQueryInformationFile)(
  IN HANDLE               FileHandle,
  OUT PIO_STATUS_BLOCK    IoStatusBlock,
  OUT PVOID               FileInformation,
  IN ULONG                Length,
  IN FILE_INFORMATION_CLASS FileInformationClass
);

//
// flags for IDN enable/disable via INTERNET_OPTION_IDN
//

#define INTERNET_FLAG_IDN_DIRECT        0x00000001  // IDN enabled for direct connections
#define INTERNET_FLAG_IDN_PROXY         0x00000002  // IDN enabled for proxy

//
// flags common to open functions (not InternetOpen()):
//

#define INTERNET_FLAG_RELOAD            0x80000000  // retrieve the original item

//
// flags for InternetOpenUrl():
//

#define INTERNET_FLAG_RAW_DATA          0x40000000  // FTP/gopher find: receive the item as raw (structured) data
#define INTERNET_FLAG_EXISTING_CONNECT  0x20000000  // FTP: use existing InternetConnect handle for server if possible

//
// flags for InternetOpen():
//

#define INTERNET_FLAG_ASYNC             0x10000000  // this request is asynchronous (where supported)

//
// protocol-specific flags:
//

#define INTERNET_FLAG_PASSIVE           0x08000000  // used for FTP connections

//
// additional cache flags
//

#define INTERNET_FLAG_NO_CACHE_WRITE    0x04000000  // don't write this item to the cache
#define INTERNET_FLAG_DONT_CACHE        INTERNET_FLAG_NO_CACHE_WRITE
#define INTERNET_FLAG_MAKE_PERSISTENT   0x02000000  // make this item persistent in cache
#define INTERNET_FLAG_FROM_CACHE        0x01000000  // use offline semantics
#define INTERNET_FLAG_OFFLINE           INTERNET_FLAG_FROM_CACHE

//
// additional flags
//

#define INTERNET_FLAG_SECURE            0x00800000  // use PCT/SSL if applicable (HTTP)
#define INTERNET_FLAG_KEEP_CONNECTION   0x00400000  // use keep-alive semantics
#define INTERNET_FLAG_NO_AUTO_REDIRECT  0x00200000  // don't handle redirections automatically
#define INTERNET_FLAG_READ_PREFETCH     0x00100000  // do background read prefetch
#define INTERNET_FLAG_NO_COOKIES        0x00080000  // no automatic cookie handling
#define INTERNET_FLAG_NO_AUTH           0x00040000  // no automatic authentication handling
#define INTERNET_FLAG_RESTRICTED_ZONE   0x00020000  // apply restricted zone policies for cookies, auth
#define INTERNET_FLAG_CACHE_IF_NET_FAIL 0x00010000  // return cache file if net request fails

//
// Security Ignore Flags, Allow HttpOpenRequest to overide
//  Secure Channel (SSL/PCT) failures of the following types.
//

#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP   0x00008000 // ex: https:// to http://
#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS  0x00004000 // ex: http:// to https://
#define INTERNET_FLAG_IGNORE_CERT_DATE_INVALID  0x00002000 // expired X509 Cert.
#define INTERNET_FLAG_IGNORE_CERT_CN_INVALID    0x00001000 // bad common name in X509 Cert.

//
// more caching flags
//

#define INTERNET_FLAG_RESYNCHRONIZE     0x00000800  // asking wininet to update an item if it is newer
#define INTERNET_FLAG_HYPERLINK         0x00000400  // asking wininet to do hyperlinking semantic which works right for scripts
#define INTERNET_FLAG_NO_UI             0x00000200  // no cookie popup
#define INTERNET_FLAG_PRAGMA_NOCACHE    0x00000100  // asking wininet to add "pragma: no-cache"
#define INTERNET_FLAG_CACHE_ASYNC       0x00000080  // ok to perform lazy cache-write
#define INTERNET_FLAG_FORMS_SUBMIT      0x00000040  // this is a forms submit
#define INTERNET_FLAG_FWD_BACK          0x00000020  // fwd-back button op
#define INTERNET_FLAG_NEED_FILE         0x00000010  // need a file for this request
#define INTERNET_FLAG_MUST_CACHE_REQUEST INTERNET_FLAG_NEED_FILE

//
// flags for FTP
//

#define INTERNET_FLAG_TRANSFER_ASCII    FTP_TRANSFER_TYPE_ASCII     // 0x00000001
#define INTERNET_FLAG_TRANSFER_BINARY   FTP_TRANSFER_TYPE_BINARY    // 0x00000002


#define INTERNET_OPTION_CONNECT_TIMEOUT             2
#define INTERNET_OPTION_CONTROL_SEND_TIMEOUT        5
#define INTERNET_OPTION_CONTROL_RECEIVE_TIMEOUT     6
#define INTERNET_OPTION_DATA_SEND_TIMEOUT           7
#define INTERNET_OPTION_DATA_RECEIVE_TIMEOUT        8

typedef HINTERNET (WINAPI *InternetOpen_f)(
  __in  LPCTSTR lpszAgent,
  __in  DWORD dwAccessType,
  __in  LPCTSTR lpszProxyName,
  __in  LPCTSTR lpszProxyBypass,
  __in  DWORD dwFlags
);


typedef HINTERNET (WINAPI *InternetConnect_f)(
  __in  HINTERNET hInternet,
  __in  LPCTSTR lpszServerName,
  __in  INTERNET_PORT nServerPort,
  __in  LPCTSTR lpszUsername,
  __in  LPCTSTR lpszPassword,
  __in  DWORD dwService,
  __in  DWORD dwFlags,
  __in  DWORD_PTR dwContext
);


typedef BOOL (WINAPI *InternetCloseHandle_f)(
  __in  HINTERNET hInternet
);


typedef HINTERNET (WINAPI *HttpOpenRequest_f)(
  __in  HINTERNET hConnect,
  __in  LPCTSTR lpszVerb,
  __in  LPCTSTR lpszObjectName,
  __in  LPCTSTR lpszVersion,
  __in  LPCTSTR lpszReferer,
  __in  LPCTSTR *lplpszAcceptTypes,
  __in  DWORD dwFlags,
  __in  DWORD_PTR dwContext
);


typedef BOOL (WINAPI *HttpSendRequest_f)(
  __in  HINTERNET hRequest,
  __in  LPCTSTR lpszHeaders,
  __in  DWORD dwHeadersLength,
  __in  LPVOID lpOptional,
  __in  DWORD dwOptionalLength
);

typedef BOOL (WINAPI *InternetCrackUrl_f)(
  __in     LPCTSTR lpszUrl,
  __in     DWORD dwUrlLength,
  __in     DWORD dwFlags,
  __inout  LPURL_COMPONENTSA lpUrlComponents
);

typedef BOOL (WINAPI *InternetReadFile_f)(
  __in   HINTERNET hFile,
  __out  LPVOID lpBuffer,
  __in   DWORD dwNumberOfBytesToRead,
  __out  LPDWORD lpdwNumberOfBytesRead
);

typedef BOOL (WINAPI *InternetSetOption_f)(
  __in  HINTERNET hInternet,
  __in  DWORD dwOption,
  __in  LPVOID lpBuffer,
  __in  DWORD dwBufferLength
);

typedef BOOL (WINAPI *InternetQueryDataAvailable_f)(
  __in   HINTERNET hFile,
  __out  LPDWORD lpdwNumberOfBytesAvailable,
  __in   DWORD dwFlags,
  __in   DWORD_PTR dwContext
);

typedef NTSTATUS (WINAPI *NtQuerySystemTime_f)(
  __out  PLARGE_INTEGER SystemTime
);



typedef BOOL (WINAPI *GetSystemTimes_f)(
  __out OPTIONAL LPFILETIME lpIdleTime,
  __out OPTIONAL LPFILETIME lpKernelTime,
  __out OPTIONAL LPFILETIME lpUserTime
);

//
// access types for InternetOpen()
//

#define INTERNET_OPEN_TYPE_PRECONFIG                    0   // use registry configuration
#define INTERNET_OPEN_TYPE_DIRECT                       1   // direct to net
#define INTERNET_OPEN_TYPE_PROXY                        3   // via named proxy
#define INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY  4   // prevent using java/script/INS


//
// service types for InternetConnect()
//

#define INTERNET_SERVICE_FTP    1
#define INTERNET_SERVICE_GOPHER 2
#define INTERNET_SERVICE_HTTP   3







#ifndef ANY_SIZE
#define ANY_SIZE 1
#endif

typedef struct _MIB_UDPROW {
    DWORD dwLocalAddr;
    DWORD dwLocalPort;
} MIB_UDPROW, *PMIB_UDPROW;

typedef struct _MIB_UDPTABLE {
    DWORD dwNumEntries;
    MIB_UDPROW table[ANY_SIZE];
} MIB_UDPTABLE, *PMIB_UDPTABLE;

typedef struct _MIB_TCPROW {
    DWORD       dwState;
    DWORD       dwLocalAddr;
    DWORD       dwLocalPort;
    DWORD       dwRemoteAddr;
    DWORD       dwRemotePort;
} MIB_TCPROW, *PMIB_TCPROW;

typedef struct _MIB_TCPTABLE {
    DWORD dwNumEntries;
    MIB_TCPROW table[ANY_SIZE];
} MIB_TCPTABLE, *PMIB_TCPTABLE;

typedef DWORD (WINAPI *GetTcpTable_f)
	(
    PMIB_TCPTABLE pTcpTable,
    PDWORD        pdwSize,
    BOOL          bOrder
    );

typedef DWORD (WINAPI *GetUdpTable_f)(
    OUT    PMIB_UDPTABLE pUdpTable,
    IN OUT PDWORD        pdwSize,
    IN     BOOL          bOrder
    );

typedef LPVOID (WINAPI *ConvertThreadToFiber_f)(
    LPVOID lpParameter
);

typedef VOID (WINAPI *DeleteFiber_f)(
    LPVOID lpFiber
);


typedef enum  { 
  TCP_TABLE_BASIC_LISTENER,
  TCP_TABLE_BASIC_CONNECTIONS,
  TCP_TABLE_BASIC_ALL,
  TCP_TABLE_OWNER_PID_LISTENER,
  TCP_TABLE_OWNER_PID_CONNECTIONS,
  TCP_TABLE_OWNER_PID_ALL,
  TCP_TABLE_OWNER_MODULE_LISTENER,
  TCP_TABLE_OWNER_MODULE_CONNECTIONS,
  TCP_TABLE_OWNER_MODULE_ALL 
} TCP_TABLE_CLASS, *PTCP_TABLE_CLASS;
    
typedef enum  { 
  UDP_TABLE_BASIC,
  UDP_TABLE_OWNER_PID,
  UDP_TABLE_OWNER_MODULE 
} UDP_TABLE_CLASS, *PUDP_TABLE_CLASS;


typedef struct _MIB_UDPROW_OWNER_PID {
  DWORD dwLocalAddr;
  DWORD dwLocalPort;
  DWORD dwOwningPid;
} MIB_UDPROW_OWNER_PID, *PMIB_UDPROW_OWNER_PID;

typedef struct _MIB_UDPTABLE_OWNER_PID {
  DWORD                dwNumEntries;
  MIB_UDPROW_OWNER_PID table[ANY_SIZE];
} MIB_UDPTABLE_OWNER_PID, *PMIB_UDPTABLE_OWNER_PID;
    
typedef struct _MIB_TCPROW_OWNER_PID {
  DWORD dwState;
  DWORD dwLocalAddr;
  DWORD dwLocalPort;
  DWORD dwRemoteAddr;
  DWORD dwRemotePort;
  DWORD dwOwningPid;
} MIB_TCPROW_OWNER_PID, *PMIB_TCPROW_OWNER_PID;


typedef struct {
  DWORD                dwNumEntries;
  MIB_TCPROW_OWNER_PID table[ANY_SIZE];
} MIB_TCPTABLE_OWNER_PID, *PMIB_TCPTABLE_OWNER_PID;


typedef DWORD (WINAPI *GetExtendedTcpTable_f)(
  PVOID pTcpTable,
  DWORD* pdwSize,
  BOOL bOrder,
  ULONG ulAf,
  TCP_TABLE_CLASS TableClass,
  ULONG Reserved
);

typedef DWORD (WINAPI *GetExtendedUdpTable_f)(
  PVOID pUdpTable,
  DWORD* pdwSize,
  BOOL bOrder,
  ULONG ulAf,
  UDP_TABLE_CLASS TableClass,
  ULONG Reserved
);

typedef struct _DNS_CACHE_ENTRY {
    struct _DNS_CACHE_ENTRY* pNext; // Pointer to next entry
    PWSTR pszName; // DNS Record Name
    WORD wType; // DNS Record Type
    WORD wDataLength; // Not referenced
    DWORD dwFlags; // DNS Record Flags
} DNSCACHEENTRY, *PDNSCACHEENTRY;

typedef DWORD ( WINAPI *DnsGetCacheDataTable_f )( PDNSCACHEENTRY* pEntry );
typedef void ( WINAPI *DnsFree_f)( PVOID pData, DNS_FREE_TYPE FreeType );

typedef DNS_STATUS (WINAPI *DnsQuery_W_f )(
    PCWSTR          pszName,
    WORD            wType,
    DWORD           Options,
    PVOID           pExtra,
    PDNS_RECORDW *   ppQueryResults,
    PVOID *         pReserved
    );

typedef VOID (WINAPI *DnsRecordListFree_f)(
    PDNS_RECORDW     pRecordList,
    DNS_FREE_TYPE   FreeType
    );

typedef struct _PROCESS_MEMORY_COUNTERS {
  DWORD  cb;
  DWORD  PageFaultCount;
  SIZE_T PeakWorkingSetSize;
  SIZE_T WorkingSetSize;
  SIZE_T QuotaPeakPagedPoolUsage;
  SIZE_T QuotaPagedPoolUsage;
  SIZE_T QuotaPeakNonPagedPoolUsage;
  SIZE_T QuotaNonPagedPoolUsage;
  SIZE_T PagefileUsage;
  SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS, *PPROCESS_MEMORY_COUNTERS;

typedef BOOL (WINAPI *GetProcessMemoryInfo_f)(
  HANDLE Process,
  PPROCESS_MEMORY_COUNTERS ppsmemCounters,
  DWORD cb
);

typedef DWORD (WINAPI *GetModuleFileNameExW_f)(
  HANDLE hProcess,
  HMODULE hModule,
  LPWSTR lpFilename,
  DWORD nSize
);

typedef DWORD (WINAPI *GetProcessImageFileName_f)(
  HANDLE hProcess,
  LPWSTR lpImageFileName,
  DWORD nSize
);

typedef struct _CERT_STRONG_SIGN_SERIALIZED_INFO {
  DWORD  dwFlags;
  LPWSTR pwszCNGSignHashAlgids;
  LPWSTR pwszCNGPubKeyMinBitLengths;
} CERT_STRONG_SIGN_SERIALIZED_INFO, *PCERT_STRONG_SIGN_SERIALIZED_INFO;

typedef struct _CERT_STRONG_SIGN_PARA {
  DWORD cbSize;
  DWORD dwInfoChoice;
  union {
    void                              *pvInfo;
    PCERT_STRONG_SIGN_SERIALIZED_INFO pSerializedInfo;
    LPSTR                             pszOID;
  } DUMMYUNIONNAME;
} CERT_STRONG_SIGN_PARA, *PCERT_STRONG_SIGN_PARA;typedef const CERT_STRONG_SIGN_PARA *PCCERT_STRONG_SIGN_PARA;

typedef struct WINTRUST_SIGNATURE_SETTINGS_ {
  DWORD                  cbStruct;
  DWORD                  dwIndex;
  DWORD                  dwFlags;
  DWORD                  cSecondarySigs;
  DWORD                  dwVerifiedSigIndex;
  PCERT_STRONG_SIGN_PARA pCryptoPolicy;
} WINTRUST_SIGNATURE_SETTINGS, *PWINTRUST_SIGNATURE_SETTINGS;

typedef struct WINTRUST_FILE_INFO_ {
  DWORD   cbStruct;
  LPCWSTR pcwszFilePath;
  HANDLE  hFile;
  GUID    *pgKnownSubject;
} WINTRUST_FILE_INFO, *PWINTRUCT_FILE_INFO;

//////////////////////////////////////////////////////////////////////////////
//
// WINTRUST_DATA Structure
//----------------------------------------------------------------------------
//  Used when calling WinVerifyTrust to pass necessary information into
//  the Providers.
//
typedef struct _WINTRUST_DATA
{
    DWORD           cbStruct;                   // = sizeof(WINTRUST_DATA)

    LPVOID          pPolicyCallbackData;        // optional: used to pass data between the app and policy
    LPVOID          pSIPClientData;             // optional: used to pass data between the app and SIP.

    DWORD           dwUIChoice;                 // required: UI choice.  One of the following.
#define WTD_UI_ALL      1
#define WTD_UI_NONE     2
#define WTD_UI_NOBAD    3
#define WTD_UI_NOGOOD   4

    DWORD           fdwRevocationChecks;        // required: certificate revocation check options
#define WTD_REVOKE_NONE         0x00000000
#define WTD_REVOKE_WHOLECHAIN   0x00000001

    DWORD           dwUnionChoice;              // required: which structure is being passed in?
#define WTD_CHOICE_FILE         1
#define WTD_CHOICE_CATALOG      2
#define WTD_CHOICE_BLOB         3
#define WTD_CHOICE_SIGNER       4
#define WTD_CHOICE_CERT         5
    union
    {
        struct WINTRUST_FILE_INFO_      *pFile;         // individual file
        struct WINTRUST_CATALOG_INFO_   *pCatalog;      // member of a Catalog File
        struct WINTRUST_BLOB_INFO_      *pBlob;         // memory blob
        struct WINTRUST_SGNR_INFO_      *pSgnr;         // signer structure only
        struct WINTRUST_CERT_INFO_      *pCert;
    };

    DWORD           dwStateAction;                      // optional (Catalog File Processing)
#define WTD_STATEACTION_IGNORE           0x00000000
#define WTD_STATEACTION_VERIFY           0x00000001
#define WTD_STATEACTION_CLOSE            0x00000002
#define WTD_STATEACTION_AUTO_CACHE       0x00000003
#define WTD_STATEACTION_AUTO_CACHE_FLUSH 0x00000004

    HANDLE          hWVTStateData;                      // optional (Catalog File Processing)

    WCHAR           *pwszURLReference;          // optional: (future) used to determine zone.

    // 17-Feb-1998 philh: added
    DWORD           dwProvFlags;
#define WTD_PROV_FLAGS_MASK                      0x0000FFFF
#define WTD_USE_IE4_TRUST_FLAG                   0x00000001
#define WTD_NO_IE4_CHAIN_FLAG                    0x00000002
#define WTD_NO_POLICY_USAGE_FLAG                 0x00000004
#define WTD_REVOCATION_CHECK_NONE                0x00000010
#define WTD_REVOCATION_CHECK_END_CERT            0x00000020
#define WTD_REVOCATION_CHECK_CHAIN               0x00000040
#define WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT  0x00000080
#define WTD_SAFER_FLAG                           0x00000100
#define WTD_HASH_ONLY_FLAG                       0x00000200
#define WTD_USE_DEFAULT_OSVER_CHECK              0x00000400
#define WTD_LIFETIME_SIGNING_FLAG                0x00000800

    // 07-Jan-2004 tonyschr: added
    DWORD           dwUIContext;                // optional: used to determine action text in UI
#define WTD_UICONTEXT_EXECUTE                    0
#define WTD_UICONTEXT_INSTALL                    1
} WINTRUST_DATA, *PWINTRUST_DATA;

//////////////////////////////////////////////////////////////////////////////
//
// WINTRUST_ACTION_GENERIC_VERIFY_V2 Guid  (Authenticode)
//----------------------------------------------------------------------------
//  Assigned to the pgActionID parameter of WinVerifyTrust to verify the
//  authenticity of a file/object using the Microsoft Authenticode
//  Policy Provider,
//
//          {00AAC56B-CD44-11d0-8CC2-00C04FC295EE}
//
#define WINTRUST_ACTION_GENERIC_VERIFY_V2                       \
            { 0xaac56b,                                         \
              0xcd44,                                           \
              0x11d0,                                           \
              { 0x8c, 0xc2, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee } \
            }

typedef LONG (WINAPI *WinVerifyTrust_f)(
  IN  HWND hWnd,
  IN  GUID *pgActionID,
  IN  LPVOID pWVTData
);

typedef HANDLE HCATADMIN;
typedef HANDLE HCATINFO;

typedef struct CATALOG_INFO_ {
  DWORD cbStruct;
  WCHAR wszCatalogFile[MAX_PATH];
} CATALOG_INFO;

typedef BOOL (WINAPI *CryptCATAdminAcquireContext_f)(
  OUT  HCATADMIN *phCatAdmin,
  IN   const GUID *pgSubsystem,
  IN   DWORD dwFlags
);

typedef BOOL (WINAPI *CryptCATAdminReleaseContext_f)(
  IN  HCATADMIN hCatAdmin,
  IN  DWORD dwFlags
);

typedef BOOL (WINAPI *CryptCATAdminCalcHashFromFileHandle_f)(
  IN      HANDLE hFile,
  IN OUT  DWORD *pcbHash,
  IN      BYTE *pbHash,
  IN      DWORD dwFlags
);

typedef BOOL (WINAPI *CryptCATAdminReleaseCatalogContext_f)(
  IN  HCATADMIN hCatAdmin,
  IN  HCATINFO hCatInfo,
  IN  DWORD dwFlags
);

typedef HCATINFO (WINAPI *CryptCATAdminEnumCatalogFromHash_f)(
  IN  HCATADMIN hCatAdmin,
  IN  BYTE *pbHash,
  IN  DWORD cbHash,
  IN  DWORD dwFlags,
  IN  HCATINFO *phPrevCatInfo
);

typedef BOOL (WINAPI *CryptCATCatalogInfoFromContext_f)(
  IN      HCATINFO hCatInfo,
  IN OUT  CATALOG_INFO *psCatInfo,
  IN      DWORD dwFlags
);

typedef struct _CRYPT_PROVIDER_CERT
{
    DWORD                               cbStruct;

    PCCERT_CONTEXT                      pCert;              // must have its own ref-count!

    BOOL                                fCommercial;
    BOOL                                fTrustedRoot;       // certchk policy should set this.
    BOOL                                fSelfSigned;        // set in cert provider

    BOOL                                fTestCert;          // certchk policy will set

    DWORD                               dwRevokedReason;

    DWORD                               dwConfidence;       // set in the Certificate Provider
#define  CERT_CONFIDENCE_SIG       0x10000000  // this cert
#define  CERT_CONFIDENCE_TIME      0x01000000  // issuer cert
#define  CERT_CONFIDENCE_TIMENEST  0x00100000  // this cert
#define  CERT_CONFIDENCE_AUTHIDEXT 0x00010000  // this cert
#define  CERT_CONFIDENCE_HYGIENE   0x00001000  // this cert
#define  CERT_CONFIDENCE_HIGHEST   0x11111000

    DWORD                               dwError;
    CTL_CONTEXT                         *pTrustListContext;
    BOOL                                fTrustListSignerCert;

    // The following two are only applicable to Self Signed certificates
    // residing in a CTL.
    PCCTL_CONTEXT                       pCtlContext;
    DWORD                               dwCtlError;
    BOOL                                fIsCyclic;
    PCERT_CHAIN_ELEMENT                 pChainElement;
} CRYPT_PROVIDER_CERT, *PCRYPT_PROVIDER_CERT;

typedef struct _CRYPT_PROVIDER_SGNR {
  DWORD                cbStruct;
  FILETIME             sftVerifyAsOf;
  DWORD                csCertChain;
  CRYPT_PROVIDER_CERT  *pasCertChain;
  DWORD                dwSignerType;
  CMSG_SIGNER_INFO     *psSigner;
  DWORD                dwError;
  DWORD                csCounterSigners;
  struct _CRYPT_PROVIDER_SGNR *pasCounterSigners;
  PCCERT_CHAIN_CONTEXT pChainContext;
} CRYPT_PROVIDER_SGNR, *PCRYPT_PROVIDER_SGNR;

//////////////////////////////////////////////////////////////////////////////
//
//  allocation and free function prototypes
//----------------------------------------------------------------------------
//
typedef void        *(*PFN_CPD_MEM_ALLOC)(IN DWORD cbSize);
typedef void        (*PFN_CPD_MEM_FREE)(IN void *pvMem2Free);

typedef BOOL        (*PFN_CPD_ADD_STORE)(IN struct _CRYPT_PROVIDER_DATA *pProvData,
                                         IN HCERTSTORE hStore2Add);

typedef BOOL        (*PFN_CPD_ADD_SGNR)(IN          struct _CRYPT_PROVIDER_DATA *pProvData,
                                        IN          BOOL fCounterSigner,
                                        IN OPTIONAL DWORD idxSigner,
                                        IN          struct _CRYPT_PROVIDER_SGNR *pSgnr2Add);

typedef BOOL        (*PFN_CPD_ADD_CERT)(IN          struct _CRYPT_PROVIDER_DATA *pProvData,
                                        IN          DWORD idxSigner,
                                        IN          BOOL fCounterSigner,
                                        IN OPTIONAL DWORD idxCounterSigner,
                                        IN          PCCERT_CONTEXT pCert2Add);

typedef BOOL        (*PFN_CPD_ADD_PRIVDATA)(IN struct _CRYPT_PROVIDER_DATA *pProvData,
                                            IN struct _CRYPT_PROVIDER_PRIVDATA *pPrivData2Add);

//////////////////////////////////////////////////////////////////////////////
//
//  Provider function prototypes
//----------------------------------------------------------------------------
//

//
//  entry point for the object provider
//
typedef HRESULT     (*PFN_PROVIDER_INIT_CALL)(IN OUT struct _CRYPT_PROVIDER_DATA *pProvData);

//
//  entry point for the object provider
//
typedef HRESULT     (*PFN_PROVIDER_OBJTRUST_CALL)(IN OUT struct _CRYPT_PROVIDER_DATA *pProvData);

//
//  entry point for the Signature Provider
//
typedef HRESULT     (*PFN_PROVIDER_SIGTRUST_CALL)(IN OUT struct _CRYPT_PROVIDER_DATA *pProvData);

//
//  entry point for the Certificate Provider
//
typedef HRESULT     (*PFN_PROVIDER_CERTTRUST_CALL)(IN OUT struct _CRYPT_PROVIDER_DATA *pProvData);

//
//  entry point for the Policy Provider's final call (from the trust provider)
//
typedef HRESULT     (*PFN_PROVIDER_FINALPOLICY_CALL)(IN OUT struct _CRYPT_PROVIDER_DATA *pProvData);

//
//  entry point for the Policy Provider's "dump structure" call
//
typedef HRESULT     (*PFN_PROVIDER_TESTFINALPOLICY_CALL)(IN OUT struct _CRYPT_PROVIDER_DATA *pProvData);

//
//  entry point for the Policy Provider's clean up routine for any PRIVDATA allocated
//
typedef HRESULT     (*PFN_PROVIDER_CLEANUP_CALL)(IN OUT struct _CRYPT_PROVIDER_DATA *pProvData);

//
//  entry point for the Policy Provider's Cert Check call.  This will return
//  true if the Trust Provider is to continue building the certificate chain.
//  If the PP returns FALSE, it is assumed that we have reached a "TRUSTED",
//  self-signed, root.  it is also the CertCheck's responsibility to set the
//  fTrustedRoot flag in the certificate structure.
//
typedef BOOL        (*PFN_PROVIDER_CERTCHKPOLICY_CALL)( IN          struct _CRYPT_PROVIDER_DATA *pProvData,
                                                        IN          DWORD idxSigner,
                                                        IN          BOOL fCounterSignerChain,
                                                        IN OPTIONAL DWORD idxCounterSigner);

typedef struct _CRYPT_PROVIDER_PRIVDATA {
  DWORD cbStruct;
  GUID  gProviderID;
  DWORD cbProvData;
  void  *pvProvData;
} CRYPT_PROVIDER_PRIVDATA, *PCRYPT_PROVIDER_PRIVDATA;

//////////////////////////////////////////////////////////////////////////////
//
// CRYPT_PROVIDER_DATA Structure
//----------------------------------------------------------------------------
//  Used to pass information between WinVerifyTrust and all of the Provider
//  calls.
//
//  IMPORTANT:  1.  All dynamically allocated members MUST use the allocation
//                  and Add2 functions provided.
//
typedef struct _CRYPT_PROVIDER_DATA
{
    DWORD                               cbStruct;               // = sizeof(TRUST_PROVIDER_DATA) (set in WVT)
    WINTRUST_DATA                       *pWintrustData;         // NOT verified (set in WVT)
    BOOL                                fOpenedFile;            // the provider opened the file handle (if applicable)
    HWND                                hWndParent;             // if passed in, else, Desktop hWnd (set in WVT).
    GUID                                *pgActionID;            // represents the Provider combination (set in WVT).
    HCRYPTPROV                          hProv;                  // set to NULL to let CryptoAPI to assign.
    DWORD                               dwError;                // error if a low-level, system error was encountered
    DWORD                               dwRegSecuritySettings;  // ie security settings (set in WVT)
    DWORD                               dwRegPolicySettings;    // setreg settings (set in WVT)
    struct _CRYPT_PROVIDER_FUNCTIONS    *psPfns;                // set in WVT.
    DWORD                               cdwTrustStepErrors;     // set in WVT.
    DWORD                               *padwTrustStepErrors;   // allocated in WVT.  filled in WVT & Trust Provider
    DWORD                               chStores;               // number of stores in pahStores (root set in WVT)
    HCERTSTORE                          *pahStores;             // array of known stores (root set in WVT) root is ALWAYS #0!!!
    DWORD                               dwEncoding;             // message encoding type (set in WVT and Signature Prov)
    HCRYPTMSG                           hMsg;                   // set in Signature Prov.
    DWORD                               csSigners;              // use Add2 and Get functions!
    struct _CRYPT_PROVIDER_SGNR         *pasSigners;            // use Add2 and Get functions!
    DWORD                               csProvPrivData;         // use Add2 and Get functions!
    struct _CRYPT_PROVIDER_PRIVDATA     *pasProvPrivData;       // use Add2 and Get functions!
    DWORD                               dwSubjectChoice;
#define CPD_CHOICE_SIP 1

    union
    {
        struct _PROVDATA_SIP            *pPDSip;
    };

    char                                *pszUsageOID;           // set in Init Provider
    BOOL                                fRecallWithState;       // state was maintained for Catalog Files.
    FILETIME                            sftSystemTime;
    char                                *pszCTLSignerUsageOID;

    // 17-Feb-1998 philh: added
    // LOWORD intialized from WINTRUST_DATA's dwProvFlags.
    DWORD                               dwProvFlags;
#define CPD_USE_NT5_CHAIN_FLAG                   0x80000000
#define CPD_REVOCATION_CHECK_NONE                0x00010000
#define CPD_REVOCATION_CHECK_END_CERT            0x00020000
#define CPD_REVOCATION_CHECK_CHAIN               0x00040000
#define CPD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT  0x00080000

    DWORD                               dwFinalError;
    PCERT_USAGE_MATCH					pRequestUsage;
    DWORD                               dwTrustPubSettings; 
    DWORD                               dwUIStateFlags;
#define CPD_UISTATE_MODE_PROMPT 0x00000000
#define CPD_UISTATE_MODE_BLOCK  0x00000001
#define CPD_UISTATE_MODE_ALLOW  0x00000002
#define CPD_UISTATE_MODE_MASK   0x00000003

} CRYPT_PROVIDER_DATA, *PCRYPT_PROVIDER_DATA;

//////////////////////////////////////////////////////////////////////////////
//
// CRYPT_PROVIDER_FUNCTIONS structure
//----------------------------------------------------------------------------
//
typedef struct _CRYPT_PROVIDER_FUNCTIONS
{
    DWORD                               cbStruct;

    PFN_CPD_MEM_ALLOC                   pfnAlloc;               // set in WVT
    PFN_CPD_MEM_FREE                    pfnFree;                // set in WVT

    PFN_CPD_ADD_STORE                   pfnAddStore2Chain;      // call to add a store to the chain.
    PFN_CPD_ADD_SGNR                    pfnAddSgnr2Chain;       // call to add a sgnr struct to a msg struct sgnr chain
    PFN_CPD_ADD_CERT                    pfnAddCert2Chain;       // call to add a cert struct to a sgnr struct cert chain
    PFN_CPD_ADD_PRIVDATA                pfnAddPrivData2Chain;   // call to add provider private data to struct.

    PFN_PROVIDER_INIT_CALL              pfnInitialize;          // initialize Policy data.
    PFN_PROVIDER_OBJTRUST_CALL          pfnObjectTrust;         // build info up to the signer info(s).
    PFN_PROVIDER_SIGTRUST_CALL          pfnSignatureTrust;      // build info to the signing cert
    PFN_PROVIDER_CERTTRUST_CALL         pfnCertificateTrust;    // build the chain
    PFN_PROVIDER_FINALPOLICY_CALL       pfnFinalPolicy;         // final call to policy
    PFN_PROVIDER_CERTCHKPOLICY_CALL     pfnCertCheckPolicy;     // check each cert will building chain
    PFN_PROVIDER_TESTFINALPOLICY_CALL   pfnTestFinalPolicy;     // dump structures to a file (or whatever the policy chooses)

    struct _CRYPT_PROVUI_FUNCS          *psUIpfns;

                    // 23-Jul-1997 pberkman: added
    PFN_PROVIDER_CLEANUP_CALL           pfnCleanupPolicy;       // PRIVDATA cleanup routine.

} CRYPT_PROVIDER_FUNCTIONS, *PCRYPT_PROVIDER_FUNCTIONS;

//////////////////////////////////////////////////////////////////////////////
//
// CRYPT_PROVUI_FUNCS structure
//----------------------------------------------------------------------------
//
typedef BOOL (*PFN_PROVUI_CALL)(IN HWND hWndSecurityDialog, IN struct _CRYPT_PROVIDER_DATA *pProvData);

typedef struct _CRYPT_PROVUI_FUNCS
{
    DWORD                               cbStruct;

    struct _CRYPT_PROVUI_DATA           *psUIData;

    PFN_PROVUI_CALL                     pfnOnMoreInfoClick;
    PFN_PROVUI_CALL                     pfnOnMoreInfoClickDefault;

    PFN_PROVUI_CALL                     pfnOnAdvancedClick;
    PFN_PROVUI_CALL                     pfnOnAdvancedClickDefault;

} CRYPT_PROVUI_FUNCS, *PCRYPT_PROVUI_FUNCS;

//////////////////////////////////////////////////////////////////////////////
//
// CRYPT_PROVUI_DATA
//----------------------------------------------------------------------------
//
typedef struct _CRYPT_PROVUI_DATA
{
    DWORD                               cbStruct;

    DWORD                               dwFinalError;

    WCHAR                               *pYesButtonText;        // default: "&Yes"
    WCHAR                               *pNoButtonText;         // default: "&No"
    WCHAR                               *pMoreInfoButtonText;   // default: "&More Info"
    WCHAR                               *pAdvancedLinkText;     // default: <none>

    // 15-Sep-1997 pberkman: added
        // good: default:
                // "Do you want to install and run ""%1"" signed on %2 and distributed by:"
    WCHAR                               *pCopyActionText;
        // good no time stamp: default:
                // "Do you want to install and run ""%1"" signed on an unknown date/time and distributed by:"
    WCHAR                               *pCopyActionTextNoTS;
        // bad: default:
                // "Do you want to install and run ""%1""?"
    WCHAR                               *pCopyActionTextNotSigned;


} CRYPT_PROVUI_DATA, *PCRYPT_PROVUI_DATA;

typedef CRYPT_PROVIDER_DATA* (WINAPI *WTHelperProvDataFromStateData_f)(
  IN  HANDLE hStateData
);

typedef CRYPT_PROVIDER_SGNR* (WINAPI *WTHelperGetProvSignerFromChain_f)(
  IN  CRYPT_PROVIDER_DATA *pProvData,
  IN  DWORD idxSigner,
  IN  BOOL  fCounterSigner,
  IN  DWORD idxCounterSigner
);

typedef DWORD (WINAPI *CertNameToStr_f)(
  IN   DWORD dwCertEncodingType,
  IN   PCERT_NAME_BLOB pName,
  IN   DWORD dwStrType,
  OUT  LPTSTR psz,
  OUT   DWORD csz
);

//+-------------------------------------------------------------------------
//  Predefined verify chain policies
//--------------------------------------------------------------------------
#define R_CERT_CHAIN_POLICY_BASE              ((LPCSTR) 1)
#define R_CERT_CHAIN_POLICY_AUTHENTICODE      ((LPCSTR) 2)
#define R_CERT_CHAIN_POLICY_AUTHENTICODE_TS   ((LPCSTR) 3)
#define R_CERT_CHAIN_POLICY_SSL               ((LPCSTR) 4)
#define R_CERT_CHAIN_POLICY_BASIC_CONSTRAINTS ((LPCSTR) 5)
#define R_CERT_CHAIN_POLICY_NT_AUTH           ((LPCSTR) 6)
#define R_CERT_CHAIN_POLICY_MICROSOFT_ROOT    ((LPCSTR) 7)

typedef BOOL (WINAPI *CertVerifyCertificateChainPolicy_f)(
  IN      LPCSTR pszPolicyOID,
  IN      PCCERT_CHAIN_CONTEXT pChainContext,
  IN      PCERT_CHAIN_POLICY_PARA pPolicyPara,
  IN OUT  PCERT_CHAIN_POLICY_STATUS pPolicyStatus
);

typedef BOOL (WINAPI *CertGetCertificateChain_f)(
  IN      HCERTCHAINENGINE hChainEngine,
  IN      PCCERT_CONTEXT pCertContext,
  IN      LPFILETIME pTime,
  IN      HCERTSTORE hAdditionalStore,
  IN      PCERT_CHAIN_PARA pChainPara,
  IN      DWORD dwFlags,
  IN      LPVOID pvReserved,
  OUT     PCCERT_CHAIN_CONTEXT *ppChainContext
);

typedef BOOL (WINAPI *CertCreateCertificateChainEngine_f)(
  IN   PCERT_CHAIN_ENGINE_CONFIG pConfig,
  OUT  HCERTCHAINENGINE *phChainEngine
);

typedef void (WINAPI *CertFreeCertificateChainEngine_f)(
  IN  HCERTCHAINENGINE hChainEngine
);

typedef void (WINAPI *CertFreeCertificateChain_f)(
  IN  PCCERT_CHAIN_CONTEXT pChainContext
);

typedef BOOL (WINAPI *StackWalk64_f)(
    DWORD MachineType, 
    HANDLE hProcess, 
    HANDLE hThread, 
    LPSTACKFRAME64 StackFrame,
    PVOID ContextRecord, 
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
);

typedef	BOOL (WINAPI *SymGetSymFromAddr64_f)(
    HANDLE hProcess, 
    DWORD64 Address,
    PDWORD64 Displacement, 
    PIMAGEHLP_SYMBOL64 Symbol
);

typedef	BOOL (WINAPI *SymInitialize_f)(
    HANDLE hProcess, 
    LPCSTR UserSearchPath, 
    BOOL fInvadeProcess
);

typedef	DWORD (WINAPI *UndecorateSymbolName_f) (
    PCTSTR DecoratedName, 
    PTSTR UnDecoratedName, 
    DWORD UndecoratedLength, 
    DWORD Flags
);

typedef PVOID (WINAPI *SymFunctionTableAccess64_f)(
    HANDLE hProcess, 
    DWORD64 AddrBase
);

typedef DWORD64 (WINAPI *SymGetModuleBase64_f)(
    HANDLE hProcess, 
    DWORD64 qwAddr
);

typedef BOOL (WINAPI *SymCleanup_f)(
  HANDLE hProcess
);


#endif // _UNDOCUMENTED_H