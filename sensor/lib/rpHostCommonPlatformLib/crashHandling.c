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

#include "crashHandling.h"
#include <rpal/rpal.h>
#include "obfuscated.h"
#include <obfuscationLib/obfuscationLib.h>
#include <rpHostCommonPlatformLib/rTags.h>
#include <processLib/processLib.h>

#ifdef RPAL_PLATFORM_WINDOWS
#include <windows.h>
#include <DbgHelp.h>
#endif

#define RPAL_FILE_ID    52


RBOOL
    acquireCrashContextPresent
    (
        RPU8* pCrashContext,
        RU32* pCrashContextSize
    )
{
    RBOOL isPresent = FALSE;

    RPU8 storeFile = NULL;
    RU32 storeFileSize = 0;

    OBFUSCATIONLIB_DECLARE( store, RP_HCP_CONFIG_CRASH_STORE );

    OBFUSCATIONLIB_TOGGLE( store );

    storeFileSize = rpal_file_getSizew( (RPWCHAR)store, FALSE );

    if( (RU32)(-1) != storeFileSize )
    {
        rpal_debug_warning( "found a crash context on disk" );
        isPresent = TRUE;

        if( NULL != pCrashContext &&
            NULL != pCrashContextSize )
        {
            if( rpal_file_readw( (RPWCHAR)store, (RPVOID)&storeFile, &storeFileSize, FALSE ) )
            {
                rpal_debug_info( "acquired the crash context" );

                *pCrashContext = storeFile;
                *pCrashContextSize = storeFileSize;
            }
        }

        cleanCrashContext();
    }

    OBFUSCATIONLIB_TOGGLE( store );

    return isPresent;
}

RBOOL
    setCrashContext
    (
        RPU8 crashContext,
        RU32 crashContextSize
    )
{
    RBOOL isSet = FALSE;

    OBFUSCATIONLIB_DECLARE( store, RP_HCP_CONFIG_CRASH_STORE );

    OBFUSCATIONLIB_TOGGLE( store );

    if( NULL != crashContext &&
        0 != crashContextSize )
    {
        if( rpal_file_writew( (RPWCHAR)store, crashContext, crashContextSize, TRUE ) )
        {
            rpal_debug_info( "crash context successfully set with %d bytes", crashContextSize );
            isSet = TRUE;
        }
        else
        {
            rpal_debug_warning( "could not set crash context" );
        }
    }

    OBFUSCATIONLIB_TOGGLE( store );

    return isSet;
}


RBOOL
    cleanCrashContext
    (

    )
{
    RBOOL isSuccess = FALSE;

    OBFUSCATIONLIB_DECLARE( store, RP_HCP_CONFIG_CRASH_STORE );

    OBFUSCATIONLIB_TOGGLE( store );

    if( rpal_file_deletew( (RPWCHAR)store, TRUE ) )
    {
        rpal_debug_info( "crash context deleted from disk" );
        isSuccess = TRUE;
    }
    else
    {
        rpal_debug_warning( "could not wipe the crash context from disk" );
    }

    OBFUSCATIONLIB_TOGGLE( store );

    return isSuccess;
}

RU32
    getCrashContextSize
    (

    )
{
    RU32 size = (RU32)(-1);

    OBFUSCATIONLIB_DECLARE( store, RP_HCP_CONFIG_CRASH_STORE );

    OBFUSCATIONLIB_TOGGLE( store );

    size = rpal_file_getSizew( (RPWCHAR)store, FALSE );
    rpal_debug_info( "crash context size: %d bytes", size );
    OBFUSCATIONLIB_TOGGLE( store );

    return size;
}

#ifdef RPAL_PLATFORM_WINDOWS

RPCHAR 
    seDescription
    ( 
        const RU32 code 
    )
{
    switch( code ) 
    {
        case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION"         ;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"    ;
        case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT"               ;
        case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT"    ;
        case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND"     ;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO"       ;
        case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT"       ;
        case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION"    ;
        case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW"             ;
        case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK"          ;
        case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW"            ;
        case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION"      ;
        case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR"            ;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO"       ;
        case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW"             ;
        case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION"      ;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION" ;
        case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION"         ;
        case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP"              ;
        case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW"           ;
        default: return "UNKNOWN EXCEPTION" ;
    }
}

rSequence 
    GetModuleInfoByAddress
    (
        RU64 addr
    )
{
    rList modules;
    rSequence mod = NULL;
    rSequence foundMod = NULL;
    RU64 modBase = 0;
    RU64 modSize = 0;

    if ( NULL != ( modules = processLib_getProcessModules( GetCurrentProcessId() ) ) )
    {
        rList_resetIterator( modules );
        while( rList_getSEQUENCE( modules, RP_TAGS_DLL, &mod ) )
        {
            if( rSequence_getPOINTER64( mod, RP_TAGS_BASE_ADDRESS, &modBase ) &&
                rSequence_getRU64( mod, RP_TAGS_MEMORY_SIZE, &modSize ) )
            {
                if( ( addr >= modBase ) && ( addr <= modBase + modSize ) )
                {
                    // we're in the right module
                    foundMod = rSequence_duplicate( mod );
                    break;
                }
            }
        }

        rList_free( modules );
    }

    return foundMod;
}

typedef BOOL (WINAPI *FpStackWalk64)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64,
                      PVOID, PREAD_PROCESS_MEMORY_ROUTINE64,
                      PFUNCTION_TABLE_ACCESS_ROUTINE64,
                      PGET_MODULE_BASE_ROUTINE64,
                      PTRANSLATE_ADDRESS_ROUTINE64);
typedef	BOOL (WINAPI *FpSymGetSymFromAddr64)(HANDLE, DWORD64,
                      PDWORD64, PIMAGEHLP_SYMBOL64);
typedef	BOOL (WINAPI *FpSymInitialize)(HANDLE, LPCSTR, BOOL);
typedef	DWORD (WINAPI *FpUndecorateSymbolName) (PCTSTR DecoratedName, PTSTR UnDecoratedName, DWORD UndecoratedLength, DWORD Flags);
typedef PVOID (WINAPI *FpSymFunctionTableAccess64)( HANDLE hProcess, DWORD64 AddrBase );
typedef DWORD64 (WINAPI *FpSymGetModuleBase64)( HANDLE hProcess, DWORD64 qwAddr );


LONG WINAPI 
    CrashHandler
    (
        PEXCEPTION_POINTERS pExceptionPtrs
    )
{
    PEXCEPTION_RECORD pER;
    PCONTEXT pContext;
    rSequence crashInfo = NULL;
    rBlob blob = NULL;
    rSequence mod = NULL;
    rList frames = NULL;
    rSequence frame = NULL;
    RCHAR fn[0x400];
    RU32 fr;
    STACKFRAME64 stack;
    PIMAGEHLP_SYMBOL64 pSymbol;
    RU64 displacement;
    RPWCHAR pModName = NULL;
    RCHAR symbolName[0x100];
    HINSTANCE hDbgHelp = NULL;
    FpStackWalk64 fpStackWalk64 = NULL;
    FpSymGetSymFromAddr64 fpSymGetSymFromAddr64 = NULL;
    FpSymInitialize fpSymInitialize = NULL;
    FpUndecorateSymbolName fpUndecorateSymbolName = NULL;
    FpSymFunctionTableAccess64 fpSymFunctionTableAccess64 = NULL;
    FpSymGetModuleBase64 fpSymGetModuleBase64 = NULL;
    
    pER = pExceptionPtrs->ExceptionRecord;
    pContext = pExceptionPtrs->ContextRecord;
    rpal_debug_critical( "Entered Global Exception Handler" );
    if ( NULL != ( crashInfo = rSequence_new() ) )
    {
        if ( NULL != ( blob = rpal_blob_create( 0, 0 ) ) )
        {
            rSequence_addSTRINGA( crashInfo, RP_TAGS_CRASH_EXCEPTION_CODE, seDescription( pER->ExceptionCode ) );
            rpal_debug_critical( "Exception: %s at address 0x%08llX", seDescription( pER->ExceptionCode ), ( RU64 ) pER->ExceptionAddress );
            
            if ( NULL != ( mod = GetModuleInfoByAddress( ( RU64 ) pER->ExceptionAddress ) ) )
            {
                if ( rSequence_getSTRINGW( mod, RP_TAGS_MODULE_NAME, &pModName ) )
                {
                    rSequence_addSTRINGW( crashInfo, RP_TAGS_MODULE_NAME, pModName );
                    rpal_debug_critical( "In module %ls.", pModName );
                }

                rSequence_free( mod );
                mod = NULL;
            }
#ifndef WIN64
            rSequence_addPOINTER32( crashInfo, RP_TAGS_CRASH_ADDRESS, (RU32) pER->ExceptionAddress );
            sprintf( fn, "EAX = 0x%08X, EBX = 0x%08X, ECX = 0x%08X, EDX = 0x%08X, EDI = 0x%08X, ESI = 0x%08X, EBP = 0x%08X, ESP = 0x%08X, EIP = 0x%08X", 
                pContext->Eax, pContext->Ebx, pContext->Ecx, pContext->Edx, pContext->Edi, pContext->Esi, pContext->Ebp, pContext->Esp, pContext->Eip );
#else 
            rSequence_addPOINTER64( crashInfo, RP_TAGS_CRASH_ADDRESS, (RU64) pER->ExceptionAddress );
            sprintf( fn, "Rax = 0x%016llX, Rbx  = 0x%016llX, Rcx = 0x%016llX, Rdx = 0x%016llx, Rdi = 0x%016llx, Rsi = 0x%016llx, Rbp = 0x%016llx, Rsp = 0x%016llx, Rip = 0x%016llx, R8 = 0x%016llx, R9 = 0x%016llx, R10 = 0x%016llx, R11 = 0x%016llx, R12 = 0x%016llx, R13 = 0x%016llx, R14 = 0x%016llx, R15 = 0x%016llx",
                pContext->Rax, pContext->Rbx, pContext->Rcx, pContext->Rdx, pContext->Rdi, pContext->Rsi, pContext->Rbp, pContext->Rsp, pContext->Rip, pContext->R8, pContext->R9, pContext->R10, pContext->R11, pContext->R12, pContext->R13, pContext->R14, pContext->R15 );
#endif
            rSequence_addSTRINGA( crashInfo, RP_TAGS_CRASH_REGISTERS, fn );
            rpal_debug_critical( "%s", fn );
            // now attempt to do the stack trace  
            // before we go on, we need to get the relevant addresses from dbgHelp.dll dynamically to ensure we work on all versions of Windows...
            if ( ( NULL != ( hDbgHelp = LoadLibraryA( "DbgHelp.dll" ) ) ) && 
                ( NULL != ( fpSymInitialize = ( FpSymInitialize )GetProcAddress( hDbgHelp, "SymInitialize" ) ) ) &&
                ( NULL != ( fpStackWalk64 = ( FpStackWalk64 )GetProcAddress( hDbgHelp, "StackWalk64" ) ) ) &&
                ( NULL != ( fpSymGetSymFromAddr64 = (FpSymGetSymFromAddr64 )GetProcAddress( hDbgHelp, "SymGetSymFromAddr64" ) ) ) &&
                ( NULL != ( fpSymFunctionTableAccess64 = (FpSymFunctionTableAccess64 )GetProcAddress( hDbgHelp, "SymFunctionTableAccess64" ) ) ) &&
                ( NULL != ( fpSymGetModuleBase64 = (FpSymGetModuleBase64)GetProcAddress( hDbgHelp, "SymGetModuleBase64" ) ) ) &&
                ( NULL != ( fpUndecorateSymbolName = ( FpUndecorateSymbolName )GetProcAddress( hDbgHelp, "UnDecorateSymbolName" ) ) ) )
            {
                fpSymInitialize( GetCurrentProcess(), NULL, TRUE);
                memset( &stack, 0, sizeof( STACKFRAME64 ) );
#ifndef WIN64
                stack.AddrPC.Offset    = pContext->Eip;
                stack.AddrPC.Mode      = AddrModeFlat;
                stack.AddrStack.Offset = pContext->Esp;
                stack.AddrStack.Mode   = AddrModeFlat;
                stack.AddrFrame.Offset = pContext->Ebp;
                stack.AddrFrame.Mode   = AddrModeFlat;
#else
                stack.AddrPC.Offset    = pContext->Rip;
                stack.AddrPC.Mode      = AddrModeFlat;
                stack.AddrStack.Offset = pContext->Rsp;
                stack.AddrStack.Mode   = AddrModeFlat;
                stack.AddrFrame.Offset = pContext->Rbp;
                stack.AddrFrame.Mode   = AddrModeFlat;
#endif
                // allocate memory for the stack trace frames
                if ( NULL != ( pSymbol = ( PIMAGEHLP_SYMBOL64 ) malloc( sizeof( IMAGEHLP_SYMBOL64 ) + RPAL_MAX_PATH * sizeof( TCHAR ) ) ) && 
                     NULL != ( frames = rList_new( RP_TAGS_CRASH_STACK_TRACE_FRAME, RPCM_SEQUENCE ) ) )
                {
                    for( fr = 0; ; fr++ )
                    {
#ifndef WIN64
                        if ( fpStackWalk64( IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &stack, pContext, NULL, fpSymFunctionTableAccess64,
                                          fpSymGetModuleBase64, NULL ) && 
                             NULL != ( frame = rSequence_new() ) )
#else
                        if ( fpStackWalk64( IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), GetCurrentThread(), &stack, pContext, NULL, fpSymFunctionTableAccess64,
                                          fpSymGetModuleBase64, NULL ) && 
                             NULL != ( frame = rSequence_new() ) )
#endif
                        {
                            pSymbol->SizeOfStruct  = sizeof( IMAGEHLP_SYMBOL64 ) + RPAL_MAX_PATH * sizeof( TCHAR );
                            pSymbol->MaxNameLength = RPAL_MAX_PATH;
                            fpSymGetSymFromAddr64( GetCurrentProcess(), ( ULONG64 )stack.AddrPC.Offset, &displacement, pSymbol );
                            fpUndecorateSymbolName( pSymbol->Name, ( PSTR )symbolName, sizeof( symbolName ), UNDNAME_COMPLETE );
                            memset( fn, 0, sizeof( fn ) );
                            rpal_debug_critical( "Frame %lu: \tSymbol name:    %s\n\tPC address:     0x%016llX\n\tStack address:  0x%016llX\n\tFrame address:  0x%016llX\n",
                                fr, symbolName, ( ULONG64 )stack.AddrPC.Offset, ( ULONG64 )stack.AddrStack.Offset, ( ULONG64 )stack.AddrFrame.Offset );
                            rSequence_addSTRINGA( frame, RP_TAGS_CRASH_STACK_TRACE_FRAME_SYM_NAME, pSymbol->Name );
                            rSequence_addRU64( frame, RP_TAGS_CRASH_STACK_TRACE_FRAME_PC, ( ULONG64 )stack.AddrPC.Offset );
                            rSequence_addRU64( frame, RP_TAGS_CRASH_STACK_TRACE_FRAME_SP, ( ULONG64 )stack.AddrStack.Offset );
                            rSequence_addRU64( frame, RP_TAGS_CRASH_STACK_TRACE_FRAME_FP, ( ULONG64 )stack.AddrFrame.Offset );

                            if( !rList_addSEQUENCE( frames, frame ) )
                            {
                                rSequence_free( frame );
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    free( pSymbol );

                    if ( NULL != frames )
                    {
                        rSequence_addLIST( crashInfo, RP_TAGS_CRASH_STACK_TRACE_FRAMES, frames );
                    }
                }    
            }

            if ( rSequence_serialise( crashInfo, blob ) )
            {
                setCrashContext( ( RPU8 ) rpal_blob_getBuffer( blob ), rpal_blob_getSize( blob ) );
            }

            rpal_blob_free( blob );
        }

        rSequence_free( crashInfo );
        crashInfo = NULL;
    }

    return EXCEPTION_EXECUTE_HANDLER; 
} 
#endif

RBOOL
    setGlobalCrashHandler
    (

    )
{
    RBOOL status = FALSE;

#ifdef RPAL_PLATFORM_WINDOWS
    LPTOP_LEVEL_EXCEPTION_FILTER oldHandler;

    oldHandler = SetUnhandledExceptionFilter( CrashHandler );
//	AddVectoredExceptionHandler( 1, CrashHandler );
    SetErrorMode( SEM_NOGPFAULTERRORBOX );  // do not pop up a drWatson window!

    status = TRUE;
#endif

    return status;
}

void
    forceCrash
    (

    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    __try
    {
        RaiseException( EXCEPTION_INVALID_DISPOSITION, EXCEPTION_NONCONTINUABLE, 0, NULL );
        /*RU8 buf[10];
        RU32 i;

        for ( i = 0; i< 100; i++ )
        {
            // trash the stack...
            buf[i] = 'A';
        } */
    }
    __except( CrashHandler( GetExceptionInformation() ) )
    {

    }
#endif
}


