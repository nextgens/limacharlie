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

#include <rpal/rpal.h>
#include <rpHostCommonPlatformLib/rpHostCommonPlatformLib.h>

#ifdef RPAL_PLATFORM_LINUX
#include <signal.h>
#endif


#ifdef RPAL_PLATFORM_DEBUG
#ifndef HCP_EXE_ENABLE_MANUAL_LOAD
#define HCP_EXE_ENABLE_MANUAL_LOAD
#endif
#endif

static rEvent g_timeToQuit = NULL;



#ifdef RPAL_PLATFORM_WINDOWS
BOOL
    ctrlHandler
    (
        DWORD type
    )
{
    BOOL isHandled = FALSE;

    static RU32 isHasBeenSignaled = 0;
    
    UNREFERENCED_PARAMETER( type );

    if( 0 == rInterlocked_set32( &isHasBeenSignaled, 1 ) )
    {
        // We handle all events the same way, cleanly exit
    
        rpal_debug_info( "terminating rpHCP." );
        rpHostCommonPlatformLib_stop();
    
        rEvent_set( g_timeToQuit );
    
        isHandled = TRUE;
    }

    return isHandled;
}
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
void
    ctrlHandler
    (
        int sigNum
    )
{
    static RU32 isHasBeenSignaled = 0;
    
    if( 0 == rInterlocked_set32( &isHasBeenSignaled, 1 ) )
    {
        rpal_debug_info( "terminating rpHCP." );
        rpHostCommonPlatformLib_stop();
        rEvent_set( g_timeToQuit );
    }
}
#endif

#ifdef RPAL_PLATFORM_WINDOWS


#define _SERVICE_NAME "rphcpsvc"
#define _SERVICE_NAMEW _WCH( "rphcpsvc" )
static SERVICE_STATUS g_svc_status = { 0 };
static SERVICE_STATUS_HANDLE g_svc_status_handle = NULL;
static RU8 g_svc_conf = 0;
static RPCHAR g_svc_primary = NULL;
static RPCHAR g_svc_secondary = NULL;
static RPWCHAR g_svc_mod = NULL;

static
RU32
    installService
    (

    )
{
    HMODULE hModule = NULL;
    RWCHAR curPath[ RPAL_MAX_PATH ] = { 0 };
    RWCHAR destPath[] = _WCH( "%SYSTEMROOT%\\system32\\rphcp.exe" );
    RWCHAR svcPath[] = _WCH( "\"%SYSTEMROOT%\\system32\\rphcp.exe\" -w" );
    SC_HANDLE hScm = NULL;
    SC_HANDLE hSvc = NULL;
    RWCHAR svcName[] = { _SERVICE_NAMEW };
    RWCHAR svcDisplay[] = { _WCH( "rp_HCP_Svc" ) };

    hModule = GetModuleHandleW( NULL );
    if( NULL != hModule )
    {
        if( ARRAY_N_ELEM( curPath ) > GetModuleFileNameW( hModule, curPath, ARRAY_N_ELEM( curPath ) ) )
        {
            if( rpal_file_movew( curPath, destPath ) )
            {
                if( NULL != ( hScm = OpenSCManagerA( NULL, NULL, SC_MANAGER_CREATE_SERVICE ) ) )
                {
                    if( NULL != ( hSvc = CreateServiceW( hScm,
                                                         svcName,
                                                         svcDisplay,
                                                         SERVICE_ALL_ACCESS,
                                                         SERVICE_WIN32_OWN_PROCESS,
                                                         SERVICE_AUTO_START,
                                                         SERVICE_ERROR_NORMAL,
                                                         svcPath,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         _WCH( "" ) ) ) )
                    {
                        if( StartService( hSvc, 0, NULL ) )
                        {
                            // Emitting as error level to make sure it's displayed in release.
                            rpal_debug_error( "service installer!" );
                            return 0;
                        }
                        else
                        {
                            rpal_debug_error( "could not start service: %d", GetLastError() );
                        }

                        CloseServiceHandle( hSvc );
                    }
                    else
                    {
                        rpal_debug_error( "could not create service in SCM: %d", GetLastError() );
                    }

                    CloseServiceHandle( hScm );
                }
                else
                {
                    rpal_debug_error( "could not open SCM: %d", GetLastError() );
                }
            }
            else
            {
                rpal_debug_error( "could not move executable to service location: %d", GetLastError() );
            }
        }
        else
        {
            rpal_debug_error( "could not get current executable path: %d", GetLastError() );
        }

        CloseHandle( hModule );
    }
    else
    {
        rpal_debug_error( "could not get current executable handle: %d", GetLastError() );
    }
    
    return GetLastError();
}

static
VOID WINAPI 
    SvcCtrlHandler
    (
        DWORD fdwControl
    )
{
    switch( fdwControl )
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:

            if( g_svc_status.dwCurrentState != SERVICE_RUNNING )
                break;

            /*
            * Perform tasks necessary to stop the service here
            */

            g_svc_status.dwControlsAccepted = 0;
            g_svc_status.dwCurrentState = SERVICE_STOP_PENDING;
            g_svc_status.dwWin32ExitCode = 0;
            g_svc_status.dwCheckPoint = 2;

            SetServiceStatus( g_svc_status_handle, &g_svc_status );

            rpal_debug_info( "terminating rpHCP." );
            rpHostCommonPlatformLib_stop();

            rEvent_set( g_timeToQuit );

            break;

        default:
            break;
    }
}

static
VOID WINAPI 
    ServiceMain
    (
        DWORD  dwArgc,
        RPCHAR lpszArgv
    )
{
    RU32 memUsed = 0;
    RCHAR svcName[] = { _SERVICE_NAME };

    UNREFERENCED_PARAMETER( dwArgc );
    UNREFERENCED_PARAMETER( lpszArgv );


    if( NULL == ( g_svc_status_handle = RegisterServiceCtrlHandlerA( svcName, SvcCtrlHandler ) ) )
    {
        return;
    }

    rpal_memory_zero( &g_svc_status, sizeof( g_svc_status ) );
    g_svc_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_svc_status.dwControlsAccepted = 0;
    g_svc_status.dwCurrentState = SERVICE_START_PENDING;
    g_svc_status.dwWin32ExitCode = 0;
    g_svc_status.dwServiceSpecificExitCode = 0;
    g_svc_status.dwCheckPoint = 0;
    SetServiceStatus( g_svc_status_handle, &g_svc_status );

    if( NULL == ( g_timeToQuit = rEvent_create( TRUE ) ) )
    {
        g_svc_status.dwControlsAccepted = 0;
        g_svc_status.dwCurrentState = SERVICE_STOPPED;
        g_svc_status.dwWin32ExitCode = GetLastError();
        g_svc_status.dwCheckPoint = 1;
        SetServiceStatus( g_svc_status_handle, &g_svc_status );
        return;
    }

    rpal_debug_info( "initialising rpHCP." );
    if( !rpHostCommonPlatformLib_launch( g_svc_conf, g_svc_primary, g_svc_secondary ) )
    {
        rpal_debug_warning( "error launching hcp." );
    }

    if( NULL != g_svc_mod )
    {
#ifdef HCP_EXE_ENABLE_MANUAL_LOAD
        rpHostCommonPlatformLib_load( g_svc_mod );
#endif
        rpal_memory_free( g_svc_mod );
    }

    g_svc_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_svc_status.dwCurrentState = SERVICE_RUNNING;
    g_svc_status.dwWin32ExitCode = 0;
    g_svc_status.dwCheckPoint = 1;
    SetServiceStatus( g_svc_status_handle, &g_svc_status );

    rpal_debug_info( "...running, waiting to exit..." );
    rEvent_wait( g_timeToQuit, RINFINITE );
    rEvent_free( g_timeToQuit );

    if( rpal_memory_isValid( g_svc_mod ) )
    {
        rpal_memory_free( g_svc_mod );
    }

    rpal_debug_info( "...exiting..." );
    rpal_Context_cleanup();

    memUsed = rpal_memory_totalUsed();
    if( 0 != memUsed )
    {
        rpal_debug_critical( "Memory leak: %d bytes.\n", memUsed );
        //rpal_memory_findMemory();
#ifdef RPAL_FEATURE_MEMORY_ACCOUNTING
        rpal_memory_printDetailedUsage();
#endif
    }

    g_svc_status.dwControlsAccepted = 0;
    g_svc_status.dwCurrentState = SERVICE_STOPPED;
    g_svc_status.dwWin32ExitCode = 0;
    g_svc_status.dwCheckPoint = 3;
    SetServiceStatus( g_svc_status_handle, &g_svc_status );
}


#endif


int
RPAL_EXPORT
    main
    (
        int argc,
        char* argv[]
    )
{
    RCHAR argFlag = 0;
    RPCHAR argVal = NULL;
    RU32 conf = 0;
    RPCHAR primary = NULL;
    RPCHAR secondary = NULL;
    RPWCHAR tmpMod = NULL;
    RU32 memUsed = 0;
    RBOOL asService = FALSE;

    rpal_opt switches[] = { { 'h', "help", FALSE },
                            { 'c', "config", TRUE },
                            { 'p', "primary", TRUE },
                            { 's', "secondary", TRUE },
                            { 'm', "manual", TRUE }
#ifdef RPAL_PLATFORM_WINDOWS
                            ,
                            { 'i', "install", FALSE },
                            { 'w', "service", FALSE }
#endif
                          };

    if( rpal_initialize( NULL, 0 ) )
    {
        while( -1 != ( argFlag = rpal_getopt( argc, argv, switches, &argVal ) ) )
        {
            switch( argFlag )
            {
                case 'c':
                    rpal_string_atoi( argVal, &conf );
                    rpal_debug_info( "Setting config id: %d.", conf );
                    break;
                case 'p':
                    primary = argVal;
                    rpal_debug_info( "Setting primary URL: %s.", primary );
                    break;
                case 's':
                    secondary = argVal;
                    rpal_debug_info( "Setting secondary URL: %s.", secondary );
                    break;
                case 'm':
                    tmpMod = rpal_string_atow( argVal );
                    rpal_debug_info( "Manually loading module: %s.", argVal );
                    break;
#ifdef RPAL_PLATFORM_WINDOWS
                case 'i':
                    return installService();
                    break;
                case 'w':
                    asService = TRUE;
                    break;
#endif
                case 'h':
                default:
#ifdef RPAL_PLATFORM_DEBUG
                    printf( "Usage: %s [ -c configId ] [ -p primaryHomeUrl ] [ -s secondaryHomeUrl ] [ -m moduleToLoad ] [ -h ].\n", argv[ 0 ] );
                    printf( "-c: configuration Id used to enroll agent to different ranges as determined by the site configurations.\n" );
                    printf( "-p: primary Url used to communicate home.\n" );
                    printf( "-s: secondary Url used to communicate home if the primary failed.\n" );
                    printf( "-m: module to be loaded manually, only available in debug builds.\n" );
#ifdef RPAL_PLATFORM_WINDOWS
                    printf( "-i: install executable as a service.\n" );
                    printf( "-w: executable is running as a Windows service.\n" );
#endif
                    printf( "-h: this help.\n" );
                    return 0;
#endif
                    break;
            }
        }

#ifdef RPAL_PLATFORM_WINDOWS
        if( asService )
        {
            RCHAR svcName[] = { _SERVICE_NAME };
            SERVICE_TABLE_ENTRYA DispatchTable[] =
            {
                { NULL, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
                { NULL, NULL }
            };

            DispatchTable[ 0 ].lpServiceName = svcName;

            g_svc_conf = (RU8)conf;
            g_svc_primary = primary;
            g_svc_secondary = secondary;
            g_svc_mod = tmpMod;
            if( !StartServiceCtrlDispatcherA( DispatchTable ) )
            {
                return GetLastError();
            }
            else
            {
                return 0;
            }
        }
#endif

        rpal_debug_info( "initialising rpHCP." );
        if( !rpHostCommonPlatformLib_launch( (RU8)conf, primary, secondary ) )
        {
            rpal_debug_warning( "error launching hcp." );
        }

        if( NULL == ( g_timeToQuit = rEvent_create( TRUE ) ) )
        {
            rpal_debug_error( "error creating quit event." );
            return -1;
        }

#ifdef RPAL_PLATFORM_WINDOWS
        if( !SetConsoleCtrlHandler( (PHANDLER_ROUTINE)ctrlHandler, TRUE ) )
        {
            rpal_debug_error( "error registering control handler function." );
            return -1;
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( SIG_ERR == signal( SIGINT, ctrlHandler ) )
        {
            rpal_debug_error( "error setting signal handler" );
            return -1;
        }
#endif

        if( NULL != tmpMod )
        {
#ifdef HCP_EXE_ENABLE_MANUAL_LOAD
            rpHostCommonPlatformLib_load( tmpMod );
#endif
            rpal_memory_free( tmpMod );
        }

        rpal_debug_info( "...running, waiting to exit..." );
        rEvent_wait( g_timeToQuit, RINFINITE );
        rEvent_free( g_timeToQuit );
        
        rpal_debug_info( "...exiting..." );
        rpal_Context_cleanup();

        memUsed = rpal_memory_totalUsed();
        if( 0 != memUsed )
        {
            rpal_debug_critical( "Memory leak: %d bytes.\n", memUsed );
            //rpal_memory_findMemory();
#ifdef RPAL_FEATURE_MEMORY_ACCOUNTING
            rpal_memory_printDetailedUsage();
#endif
        }
        
        rpal_Context_deinitialize();
    }

    return 0;
}
