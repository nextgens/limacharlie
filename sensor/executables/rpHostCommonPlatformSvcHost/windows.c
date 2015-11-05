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

#include "svc_windows.h"



#define SERVICE_NAME "RPHCP"
#define SERVICE_NAME_LEGACY "RPHCP"


static SERVICE_STATUS          ServiceStatus;
static SERVICE_STATUS_HANDLE   hStatus;
static rEvent g_timeToQuit = NULL;

RVOID
    WINAPI
    ControlHandler
    ( 
        RU32 request 
    )
{
    switch( request )
    {
        case SERVICE_CONTROL_STOP:
            rpal_debug_info( "service stop requested" );
            rEvent_set( g_timeToQuit );
            ServiceStatus.dwWin32ExitCode = NO_ERROR;
            ServiceStatus.dwCheckPoint = 0;
            break;
        case SERVICE_CONTROL_SHUTDOWN:
            rpal_debug_info( "service stop requested for shutdown" );
            rEvent_set( g_timeToQuit );
            ServiceStatus.dwWin32ExitCode = NO_ERROR;
            ServiceStatus.dwCheckPoint = 0;
            break;
    }
}

RVOID
    touchStatus
    (

    )
{
    ServiceStatus.dwCheckPoint++;
    if( !SetServiceStatus( hStatus, &ServiceStatus ) )
    {
        rpal_debug_error( "could not set service status" );
    }
}

RVOID
    WINAPI
    ServiceMain
    (
        RU32 argc,
        RPCHAR* argv
    )
{
    RCHAR argFlag = 0;
    RPCHAR argVal = NULL;
    RU8 conf = 0;
    RPCHAR primary = NULL;
    RPCHAR secondary = NULL;
    RU32 memUsed = 0;

    rpal_opt switches[] = { { 'h', "help", FALSE },
                            { 'c', "config", TRUE },
                            { 'p', "primary", TRUE },
                            { 's', "secondary", TRUE },
                            { 'm', "manual", TRUE } };

    ServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_CONTROL_INTERROGATE | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode = NO_ERROR;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = ( 1 * 1000 );

    if( 0 == ( hStatus = RegisterServiceCtrlHandler( SERVICE_NAME, (LPHANDLER_FUNCTION)&ControlHandler ) ) )
    {
        hStatus = RegisterServiceCtrlHandler( SERVICE_NAME_LEGACY, (LPHANDLER_FUNCTION)&ControlHandler );
    }

    touchStatus();

    if( rpal_initialize( NULL, 0 ) )
    {
        touchStatus();

        if( NULL != ( g_timeToQuit = rEvent_create( TRUE ) ) )
        {
            touchStatus();

            while( -1 != ( argFlag = rpal_getopt( argc, argv, switches, &argVal ) ) )
            {
                switch( argFlag )
                {
                    case 'c':
                        conf = (RU8)atoi( argVal );
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
                    case 'h':
                    default:
    #ifdef RPAL_PLATFORM_DEBUG
                        printf( "Usage: %s [ -c configId ] [ -p primaryHomeUrl ] [ -s secondaryHomeUrl ] [ -m moduleToLoad ] [ -h ].", argv[ 0 ] );
                        printf( "-c: configuration Id used to enroll agent to different ranges as determined by the site configurations.\n" );
                        printf( "-p: primary Url used to communicate home.\n" );
                        printf( "-s: secondary Url used to communicate home if the primary failed.\n" );
                        printf( "-m: module to be loaded manually, only available in debug builds.\n" );
                        printf( "-h: this help.\n" );
    #endif
                        break;
                }
            }

            touchStatus();

            rpal_debug_info( "launching rpHCP" );

            if( rpHostCommonPlatformLib_launch( conf, primary, secondary ) )
            {
                ServiceStatus.dwCurrentState = SERVICE_RUNNING;
                touchStatus();

                rEvent_wait( g_timeToQuit, INFINITE );

                rpal_debug_info( "stopping rpHCP" );

                // Send the SCM an update and let it know it might be a bit long
                // since we are doing one last beacon before exiting...
                ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
                ServiceStatus.dwWaitHint = ( 20 * 1000 );
                touchStatus();

                rpHostCommonPlatformLib_stop();
            }
            else
            {
                rpal_debug_error( "rpHCP failed to launch" );
            }

            rEvent_free( g_timeToQuit );
        }
        else
        {
            rpal_debug_error( "could not create quit event" );
        }

        ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        ServiceStatus.dwWaitHint = ( 2 * 1000 );
        touchStatus();

        rpal_Context_cleanup();

        memUsed = rpal_memory_totalUsed();
        if( 0 != memUsed )
        {
            rpal_debug_critical( "Memory leak: %d bytes.\n", memUsed );
            rpal_memory_findMemory();
        }

        rpal_debug_info( "service stopped, exiting" );
        
        rpal_Context_deinitialize();

        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        ServiceStatus.dwCheckPoint = 0;
        ServiceStatus.dwWin32ExitCode = NO_ERROR;
        SetServiceStatus( hStatus, &ServiceStatus );
    }
}


BOOL 
    APIENTRY 
    DllMain
    ( 
        HMODULE hModule,
        DWORD  ul_reason_for_call,
        LPVOID lpReserved
    )
{
    UNREFERENCED_PARAMETER( hModule );
    UNREFERENCED_PARAMETER( ul_reason_for_call );
    UNREFERENCED_PARAMETER( lpReserved );
    return TRUE;
}

/*
void 
    main() 
{
   SERVICE_TABLE_ENTRY ServiceTable[ 2 ];
   ServiceTable[ 0 ].lpServiceName = SERVICE_NAME;
   ServiceTable[ 0 ].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

   ServiceTable[ 1 ].lpServiceName = NULL;
   ServiceTable[ 1 ].lpServiceProc = NULL;

   // Start the control dispatcher thread for our service
   if( 0 = =StartServiceCtrlDispatcher( ServiceTable ) )
   {
      ServiceTable[ 0 ].lpServiceName = SERVICE_NAME_LEGACY;
      StartServiceCtrlDispatcher( ServiceTable )
   }
}
*/
