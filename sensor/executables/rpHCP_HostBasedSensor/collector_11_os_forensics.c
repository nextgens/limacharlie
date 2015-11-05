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
#include <librpcm/librpcm.h>
#include "collectors.h"
#include <notificationsLib/notificationsLib.h>
#include <libOs/libOs.h>
#include <processLib/processLib.h>
#include <rpHostCommonPlatformLib/rTags.h>

#define RPAL_FILE_ID        77


static
RBOOL
_thorough_file_hash
(
RPWCHAR filePath,
RU8* pHash
)
{
    RBOOL isHashed = FALSE;

    if( NULL != filePath &&
        NULL != pHash )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        RU32 len = 0;
        RBOOL isFullPath = FALSE;
        RU32 i = 0;
        rString tmpPath = NULL;
        RPWCHAR tmpStr = NULL;
        RWCHAR winDir[] = _WCH( "%windir%" );
        RWCHAR uncPath[] = _WCH( "\\??\\" );
        RWCHAR sys32Dir[] = _WCH( "\\system32" );
        RWCHAR sysRootDir[] = _WCH( "\\systemroot" );
        RWCHAR defaultPath[] = _WCH( "%windir%\\system32\\" );
        RWCHAR defaultExt[] = _WCH( ".dll" );
        RWCHAR foundPath[ RPAL_MAX_PATH ] = { 0 };
        RU32 foundLen = 0;
        RU32 preModLen = 0;

        len = rpal_string_strlenw( filePath );

        if( 0 != len &&
            NULL != ( tmpPath = rpal_stringbuffer_new( 0, 0, TRUE ) ) )
        {
            // Check for a path token, if we have none, default to system32
            isFullPath = FALSE;
            for( i = 0; i < len; i++ )
            {
                if( _WCH( '\\' ) == filePath[ i ] ||
                    _WCH( '/' ) == filePath[ i ] )
                {
                    isFullPath = TRUE;
                    break;
                }
            }

            if( !isFullPath )
            {
                foundLen = SearchPathW( NULL, filePath, defaultExt, ARRAY_N_ELEM( foundPath ), foundPath, NULL );
                if( 0 != foundLen && ARRAY_N_ELEM( foundPath ) > foundLen )
                {
                    rpal_stringbuffer_addw( tmpPath, foundPath );
                    rpal_memory_zero( foundPath, sizeof( foundPath ) );
                }
                else
                {
                    rpal_stringbuffer_addw( tmpPath, (RPWCHAR)defaultPath );
                    rpal_stringbuffer_addw( tmpPath, filePath );
                }
            }
            else
            {
                // If the entry starts with system32, prefix it as necessary
                if( rpal_string_startswithiw( filePath, sys32Dir ) )
                {
                    rpal_stringbuffer_addw( tmpPath, winDir );
                }
                // If the entry starts with /SystemRoot, prefix it as necessary
                else if( rpal_string_startswithiw( filePath, sysRootDir ) )
                {
                    rpal_stringbuffer_addw( tmpPath, winDir );
                    filePath += rpal_string_strlenw( sysRootDir );
                }
                // If the entry starts with \??\ we can strip the UNC prefix
                else if( rpal_string_startswithiw( filePath, uncPath ) )
                {
                    filePath += rpal_string_strlenw( uncPath );
                }

                rpal_stringbuffer_addw( tmpPath, filePath );
            }

            tmpStr = rpal_stringbuffer_getStringw( tmpPath );
            len = rpal_string_strlenw( tmpStr );

            // Sometimes we deal with lists with commas, strip them
            for( i = 0; i < len; i++ )
            {
                if( _WCH( ',' ) == tmpStr[ i ] )
                {
                    tmpStr[ i ] = 0;
                }
            }

            // We remove any trailing white spaces
            rpal_string_trimw( tmpStr, _WCH( " \t" ) );

            // If this is a quoted path we will move past the first quote
            // and null-terminate at the next quote
            len = rpal_string_strlenw( tmpStr );
            preModLen = len;
            if( _WCH( '"' ) == tmpStr[ 0 ] )
            {
                tmpStr++;
                len--;
                for( i = 0; i < len; i++ )
                {
                    if( _WCH( '"' ) == tmpStr[ i ] )
                    {
                        tmpStr[ i ] = 0;
                        len = rpal_string_strlenw( tmpStr );
                        break;
                    }
                }
            }

            // Sometimes extensions are missing, default to dll
            if( 4 > len ||
                _WCH( '.' ) != tmpStr[ len - 4 ] )
            {
                rpal_stringbuffer_addw( tmpPath, (RPWCHAR)&defaultExt );
                tmpStr = rpal_stringbuffer_getStringw( tmpPath );

                if( len < preModLen )
                {
                    // The string has been shortened, so its actual end does not correspond to
                    // that of the buffer. Thus, we reconcatenate the default extension to its
                    // actual end, comfortable in the knowledge that there is enough room
                    // allocated for it (because of the prior appending to the buffer).
                    rpal_string_strcatw( tmpStr, (RPWCHAR)defaultExt );
                }
            }

            if( CryptoLib_hashFileW( tmpStr, pHash, TRUE ) )
            {
                isHashed = TRUE;
            }

            rpal_stringbuffer_free( tmpPath );
        }
#endif
    }

    return isHashed;
}


#ifdef RPAL_PLATFORM_WINDOWS

typedef struct
{
    HKEY root;
    RPWCHAR path;
    RPWCHAR keyName;
} _KeyInfo;


static
RVOID
    _enhanceAutorunsWithHashes
    (
        rList autoList
    )
{
    rSequence autoEntry = NULL;
    RPWCHAR entryExe = NULL;
    RU8 hash[ CRYPTOLIB_HASH_SIZE ] = { 0 };

    while( rList_getSEQUENCE( autoList, RP_TAGS_AUTORUN, &autoEntry ) )
    {
        if( rSequence_getSTRINGW( autoEntry, RP_TAGS_FILE_PATH, &entryExe ) )
        {
            rSequence_unTaintRead( autoEntry );

            if( _thorough_file_hash( entryExe, hash ) )
            {
                rSequence_addBUFFER( autoEntry, RP_TAGS_HASH, hash, sizeof( hash ) );
            }
        }
    }
}

static
RBOOL
    _processRegKey
    (
        DWORD type,
        RPWCHAR path,
        RPWCHAR keyName,
        RPU8 value,
        DWORD size,
        rList listEntries
    )
{
    rSequence entry = NULL;
    RPWCHAR state = NULL;
    RPWCHAR tmp = NULL;
    RU8 keyValue[ 1024 * sizeof( RWCHAR ) ] = { 0 };
    RU32 pathLen = 0;
    RBOOL isSuccess = FALSE;

    if( ( REG_SZ == type ||
        REG_EXPAND_SZ == type ) &&
        0 != size )
    {
        *(RPWCHAR)( value + size ) = 0;

        // We remove any NULL characters in the string as it's a technique used by some malware.
        rpal_string_fillw( (RPWCHAR)value, size / sizeof( RWCHAR ) - 1, _WCH( ' ' ) );

        tmp = rpal_string_strtokw( (RPWCHAR)value, _WCH( ',' ), &state );

        while( NULL != tmp &&
            0 != tmp[ 0 ] )
        {
            if( NULL != ( entry = rSequence_new() ) )
            {
                isSuccess = TRUE;
                if( rSequence_addSTRINGW( entry, RP_TAGS_FILE_PATH, (RPWCHAR)value ) )
                {
                    keyValue[ 0 ] = 0;

                    pathLen = rpal_string_strlenw( path );

                    if( sizeof( keyValue ) > ( pathLen + rpal_string_strlenw( keyName ) + rpal_string_strlenw( _WCH( "\\" ) ) ) * sizeof( RWCHAR ) )
                    {
                        rpal_string_strcpyw( (RPWCHAR)&keyValue, path );
                        if( _WCH( '\\' ) != path[ pathLen - 1 ] )
                        {
                            rpal_string_strcatw( (RPWCHAR)&keyValue, _WCH( "\\" ) );
                        }
                        rpal_string_strcatw( (RPWCHAR)&keyValue, keyName );
                        rSequence_addSTRINGW( entry, RP_TAGS_REGISTRY_KEY, (RPWCHAR)&keyValue );
                    }
                    else
                    {
                        rSequence_addSTRINGW( entry, RP_TAGS_REGISTRY_KEY, _WCH( "_" ) );
                    }

                    if( !rList_addSEQUENCE( listEntries, entry ) )
                    {
                        rSequence_free( entry );
                        entry = NULL;
                    }
                }
                else
                {
                    rSequence_free( entry );
                    entry = NULL;
                }
            }

            tmp = rpal_string_strtokw( NULL, _WCH( ',' ), &state );
            while( NULL != tmp && _WCH( ' ' ) == tmp[ 0 ] )
            {
                tmp++;
            }
        }
    }
    else if( REG_MULTI_SZ == type )
    {
        tmp = (RPWCHAR)value;
        while( (RPU8)tmp < ( (RPU8)value + size ) &&
            (RPU8)( tmp + rpal_string_strlenw( tmp ) ) <= ( (RPU8)value + size ) )
        {
            if( 0 != rpal_string_strlenw( tmp ) )
            {
                if( NULL != ( entry = rSequence_new() ) )
                {
                    isSuccess = TRUE;
                    if( rSequence_addSTRINGW( entry, RP_TAGS_FILE_PATH, tmp ) )
                    {
                        keyValue[ 0 ] = 0;
                        rpal_string_strcpyw( (RPWCHAR)&keyValue, path );
                        rpal_string_strcatw( (RPWCHAR)&keyValue, keyName );
                        rSequence_addSTRINGW( entry, RP_TAGS_REGISTRY_KEY, (RPWCHAR)&keyValue );

                        if( !rList_addSEQUENCE( listEntries, entry ) )
                        {
                            rSequence_free( entry );
                            entry = NULL;
                        }
                    }
                    else
                    {
                        rSequence_free( entry );
                        entry = NULL;
                    }
                }
            }

            tmp += rpal_string_strlenw( tmp ) + 1;
        }
    }

    return isSuccess;
}

static
RBOOL
    _getWindowsAutoruns
    (
        rList autoruns
    )
{
    RBOOL isSuccess = FALSE;

    rDirCrawl dirCrawl = NULL;
    RPWCHAR crawlFiles[] = { _WCH( "*" ), NULL };
    RPWCHAR paths[] = { _WCH( "%ALLUSERSPROFILE%\\Start Menu\\Programs\\Startup" ),
        _WCH( "%ProgramData%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup" ),
        _WCH( "%systemdrive%\\users\\*\\Start Menu\\Programs\\Startup" ),
        _WCH( "%systemdrive%\\users\\*\\AppData\\Roaming\\Microsoft\\Windows\\Start" ),
        _WCH( "%systemdrive%\\documents and settings\\*\\Start Menu\\Programs\\Startup" ),
        _WCH( "%systemdrive%\\documents and settings\\*\\AppData\\Roaming\\Microsoft\\Windows\\Start" ) };
    rFileInfo finfo = { 0 };

    RU32 i = 0;
    DWORD type = 0;
    RU8 value[ 1024 ] = { 0 };
    DWORD size = 0;
    RPWCHAR lnkDest = NULL;
    HKEY hKey = NULL;
    rSequence entry = NULL;
    RU32 iKey = 0;
    DWORD tmpKeyNameSize = 0;
    RWCHAR tmpKeyName[ 1024 ] = { 0 };
    RPWCHAR tmp = NULL;

    _KeyInfo keys[] = { { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows\\CurrentVersion\\Run\\" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Install\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Install\\Software\\Microsoft\\Windows\\CurrentVersion\\Run" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOne" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows\\CurrentVersion\\RunServices" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\" ), _WCH( "Userinit" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\" ), _WCH( "Shell" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\" ), _WCH( "Taskman" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\" ), _WCH( "System" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\" ), _WCH( "Notify" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\SpecialAccount\\Userlists" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Active Setup\\" ), _WCH( "Installed Components" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Active Setup\\" ), _WCH( "Installed Components" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "ShellExecuteHooks" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "ShellExecuteHooks" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "Browser Helper Objects" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "Browser Helper Objects" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows NT\\CurrentVersion\\" ), _WCH( "Drivers32" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\" ), _WCH( "Drivers32" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows NT\\CurrentVersion\\" ), _WCH( "Image File Execution Options" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\" ), _WCH( "Image File Execution Options" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Classes\\Exefile\\Shell\\Open\\Command\\" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows\\" ), _WCH( "Appinit_Dlls" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows\\" ), _WCH( "Appinit_Dlls" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\" ), _WCH( "SchedulingAgent" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\" ), _WCH( "Approved" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\" ), _WCH( "SvcHost" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "System\\CurrentControlSet\\Control\\Session Manager\\" ), _WCH( "AppCertDlls" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\" ), _WCH( "SecurityProviders" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SYSTEM\\CurrentControlSet\\Control\\Lsa\\" ), _WCH( "Authentication Packages" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SYSTEM\\CurrentControlSet\\Control\\Lsa\\" ), _WCH( "Notification Packages" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SYSTEM\\CurrentControlSet\\Control\\Lsa\\" ), _WCH( "Security Packages" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SYSTEM\\ControlSet00.$current.\\Control\\Session Manager\\" ), _WCH( "CWDIllegalInDllSearch" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SYSTEM\\ControlSet00.$current.\\Control\\" ), _WCH( "SafeBoot" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "System\\CurrentControlSet\\Control\\Session Manager" ), _WCH( "SetupExecute" ) },  // Values: SetupExecute, Execute, S0InitialCommand
    { HKEY_LOCAL_MACHINE, _WCH( "System\\CurrentControlSet\\Control\\Session Manager" ), _WCH( "Execute" ) },  // Values: SetupExecute, Execute, S0InitialCommand
    { HKEY_LOCAL_MACHINE, _WCH( "System\\CurrentControlSet\\Control\\Session Manager" ), _WCH( "S0InitialCommand" ) },  // Values: SetupExecute, Execute, S0InitialCommand
    { HKEY_LOCAL_MACHINE, _WCH( "System\\CurrentControlSet\\Control\\Session Manager" ), _WCH( "AppCertDlls" ) }, // Read all values of this key
    { HKEY_LOCAL_MACHINE, _WCH( "System\\CurrentControlSet\\Control" ), _WCH( "ServiceControlManagerExtension" ) },   // Value: ServiceControlManagerExtension
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Command Processor" ), _WCH( "AutoRun" ) },   // Value: AutoRun
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Microsoft\\Command Processor" ), _WCH( "AutoRun" ) },   // Value: AutoRun
    { HKEY_LOCAL_MACHINE, _WCH( "SYSTEM\\Setup\\" ), _WCH( "CmdLine" ) },   // Value: CmdLine
    { HKEY_LOCAL_MACHINE, _WCH( "SYSTEM\\Setup\\" ), _WCH( "LsaStart" ) },   // Value: LsaStart
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon" ), _WCH( "GinaDLL" ) },   // Value: GinaDLL
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon" ), _WCH( "UIHost" ) },   // Value: GinaDLL
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon" ), _WCH( "AppSetup" ) },   // Value: AppSetup
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon" ), _WCH( "VmApplet" ) },   // Value: VmApplet
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\" ), _WCH( "Shell" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\" ), _WCH( "AlternateShell" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "System\\CurrentControlSet\\Control\\SafeBoot\\Option\\" ), _WCH( "UseAlternateShell" ) },  // This one has to be 1
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows\\CurrentVersion\\RunServicesOnce" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx" ), _WCH( "*" ) },  // syntax: http://support.microsoft.com/default.aspx?scid=KB;en-us;232509
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Install\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx" ), _WCH( "*" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Policies\\Microsoft\\Windows\\System\\Scripts\\" ), _WCH( "Startup" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Policies\\Microsoft\\Windows\\System\\Scripts\\" ), _WCH( "Logon" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "System\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd\\" ), _WCH( "StartupPrograms" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\Scripts\\" ), _WCH( "Startup" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "System\\CurrentControlSet\\Control\\BootVerificationProgram" ), _WCH( "ImagePath" ) },   // Value: ImagePath
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Classes\\Protocols\\" ), _WCH( "Handler" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Internet Explorer\\Desktop\\" ), _WCH( "Components" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "Shell Folders" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Microsoft\\Windows NT\\CurrentVersion\\" ), _WCH( "Windows" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "SharedTaskScheduler" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "SharedTaskScheduler" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "ShellServiceObjects" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "ShellServiceObjects" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\" ), _WCH( "ShellServiceObjectDelayLoad" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\" ), _WCH( "ShellServiceObjectDelayLoad" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows CE Services\\" ), _WCH( "AutoStartOnConnect" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Windows CE Services\\" ), _WCH( "AutoStartOnConnect" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows CE Services\\" ), _WCH( "AutoStartOnDisconnect" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Windows CE Services\\" ), _WCH( "AutoStartOnDisconnect" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "Browser Helper Objects" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\*\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\*\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Drive\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Drive\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\*\\ShellEx\\" ), _WCH( "PropertySheetHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\*\\ShellEx\\" ), _WCH( "PropertySheetHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\AllFileSystemObjects\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\AllFileSystemObjects\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\AllFileSystemObjects\\ShellEx\\" ), _WCH( "DragDropHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\AllFileSystemObjects\\ShellEx\\" ), _WCH( "DragDropHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\AllFileSystemObjects\\ShellEx\\" ), _WCH( "PropertySheetHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\AllFileSystemObjects\\ShellEx\\" ), _WCH( "PropertySheetHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Directory\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Directory\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Directory\\ShellEx\\" ), _WCH( "DragDropHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Directory\\ShellEx\\" ), _WCH( "DragDropHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Directory\\ShellEx\\" ), _WCH( "PropertySheetHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Directory\\ShellEx\\" ), _WCH( "PropertySheetHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Directory\\ShellEx\\" ), _WCH( "CopyHookHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Directory\\ShellEx\\" ), _WCH( "CopyHookHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Directory\\Background\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Directory\\Background\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Folder\\ShellEx\\" ), _WCH( "ColumnHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Folder\\ShellEx\\" ), _WCH( "ColumnHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Folder\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Folder\\ShellEx\\" ), _WCH( "ContextMenuHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Folder\\ShellEx\\" ), _WCH( "DragDropHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Folder\\ShellEx\\" ), _WCH( "DragDropHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Folder\\ShellEx\\" ), _WCH( "ExtShellFolderViews" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Folder\\ShellEx\\" ), _WCH( "ExtShellFolderViews" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Classes\\Folder\\ShellEx\\" ), _WCH( "PropertySheetHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "Software\\Wow6432Node\\Classes\\Folder\\ShellEx\\" ), _WCH( "PropertySheetHandlers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "ShellIconOverlayIdentifiers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\" ), _WCH( "ShellIconOverlayIdentifiers" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Ctf\\" ), _WCH( "LangBarAddin" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Internet Explorer\\" ), _WCH( "UrlSearchHooks" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Internet Explorer\\" ), _WCH( "Toolbar" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Internet Explorer\\" ), _WCH( "Toolbar" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Internet Explorer\\" ), _WCH( "Explorer Bars" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Internet Explorer\\" ), _WCH( "Explorer Bars" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Microsoft\\Internet Explorer\\" ), _WCH( "Extensions" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Wow6432Node\\Microsoft\\Internet Explorer\\" ), _WCH( "Extensions" ) },
    { HKEY_LOCAL_MACHINE, _WCH( "SOFTWARE\\Classes\\" ), _WCH( "Filter" ) },

    };

    // First we check for Registry autoruns
    for( i = 0; i < ARRAY_N_ELEM( keys ); i++ )
    {
        if( ERROR_SUCCESS == RegOpenKeyW( keys[ i ].root, keys[ i ].path, &hKey ) )
        {
            if( _WCH( '*' ) != keys[ i ].keyName[ 0 ] )
            {
                // This key is a specific leaf value
                size = sizeof( value ) - sizeof( RWCHAR );
                if( ERROR_SUCCESS == RegQueryValueExW( hKey,
                    keys[ i ].keyName,
                    NULL,
                    &type,
                    (LPBYTE)&value,
                    &size ) )
                {
                    if( !_processRegKey( type, keys[ i ].path, keys[ i ].keyName, (RPU8)&value, size, autoruns ) )
                    {
                        rpal_debug_warning( "key contains unexpected data" );
                    }
                }
            }
            else
            {
                // This key is *, meaning all leaf values so we must enumerate
                tmpKeyNameSize = ARRAY_N_ELEM( tmpKeyName );
                size = sizeof( value ) - sizeof( RWCHAR );
                while( ERROR_SUCCESS == RegEnumValueW( hKey, iKey, (RPWCHAR)&tmpKeyName, &tmpKeyNameSize, NULL, &type, (RPU8)&value, &size ) )
                {
                    tmpKeyName[ tmpKeyNameSize ] = 0;

                    if( !_processRegKey( type, keys[ i ].path, tmpKeyName, (RPU8)&value, size, autoruns ) )
                    {
                        rpal_debug_warning( "key contains unexpected data" );
                    }

                    iKey++;
                    tmpKeyNameSize = ARRAY_N_ELEM( tmpKeyName );
                    size = sizeof( value ) - sizeof( RWCHAR );
                }
            }

            RegCloseKey( hKey );
        }
    }

    // Now we look for dir-based autoruns
    for( i = 0; i < ARRAY_N_ELEM( paths ); i++ )
    {
        if( NULL != ( dirCrawl = rpal_file_crawlStart( paths[ i ], crawlFiles, 1 ) ) )
        {
            while( rpal_file_crawlNextFile( dirCrawl, &finfo ) )
            {
                if( !IS_FLAG_ENABLED( finfo.attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY ) &&
                    0 != rpal_string_stricmpw( _WCH( "desktop.ini" ), finfo.fileName ) &&
                    NULL != ( entry = rSequence_new() ) )
                {
                    tmp = finfo.filePath;
                    lnkDest = NULL;

                    if( rpal_string_endswithw( finfo.filePath, _WCH( ".lnk" ) ) )
                    {
                        if( rpal_file_getLinkDest( tmp, &lnkDest ) )
                        {
                            tmp = lnkDest;
                        }
                    }

                    rSequence_addSTRINGW( entry, RP_TAGS_FILE_PATH, tmp );

                    if( NULL != lnkDest )
                    {
                        rpal_memory_free( lnkDest );
                    }

                    if( !rList_addSEQUENCE( autoruns, entry ) )
                    {
                        rSequence_free( entry );
                    }
                }
            }

            rpal_file_crawlStop( dirCrawl );
        }
    }

    isSuccess = TRUE;

    return isSuccess;
}
#endif


static
RVOID
    _enhanceServicesWithHashes
    (
        rList svcList
    )
{
    rSequence svcEntry = NULL;
    RPWCHAR entryDll = NULL;
    RPWCHAR entryExe = NULL;
    RU8 hash[ CRYPTOLIB_HASH_SIZE ] = { 0 };

    while( rList_getSEQUENCE( svcList, RP_TAGS_SVC, &svcEntry ) )
    {
        entryExe = NULL;
        entryDll = NULL;

        rSequence_getSTRINGW( svcEntry, RP_TAGS_EXECUTABLE, &entryExe );
        rSequence_getSTRINGW( svcEntry, RP_TAGS_DLL, &entryDll );

        rSequence_unTaintRead( svcEntry );

        if( NULL == entryDll &&
            NULL != entryExe )
        {
            if( _thorough_file_hash( entryExe, (RPU8)&hash ) )
            {
                rSequence_addBUFFER( svcEntry, RP_TAGS_HASH, hash, sizeof( hash ) );
            }
        }
        else if( NULL != entryDll )
        {
            if( _thorough_file_hash( entryDll, (RPU8)&hash ) )
            {
                rSequence_addBUFFER( svcEntry, RP_TAGS_HASH, hash, sizeof( hash ) );
            }
        }
    }
}


static
RVOID
    os_services
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    rList svcList;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) )
    {
        if( NULL != ( svcList = processLib_getServicesList( PROCESSLIB_SVCS ) ) )
        {
            _enhanceServicesWithHashes( svcList );

            if( !rSequence_addLIST( event, RP_TAGS_SVCS, svcList ) )
            {
                rList_free( svcList );
            }
        }
        else
        {
            rSequence_addRU32( event, RP_TAGS_ERROR, rpal_error_getLast() );
        }

        rSequence_addTIMESTAMP( event, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );
        notifications_publish( RP_TAGS_NOTIFICATION_OS_SERVICES_REP, event );
    }
}


static
RVOID
    os_drivers
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    rList svcList;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) )
    {
        if( NULL != ( svcList = processLib_getServicesList( PROCESSLIB_DRIVERS ) ) )
        {
            _enhanceServicesWithHashes( svcList );

            if( !rSequence_addLIST( event, RP_TAGS_SVCS, svcList ) )
            {
                rList_free( svcList );
            }
        }
        else
        {
            rSequence_addRU32( event, RP_TAGS_ERROR, rpal_error_getLast() );
        }

        rSequence_addTIMESTAMP( event, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );
        notifications_publish( RP_TAGS_NOTIFICATION_OS_DRIVERS_REP, event );
    }
}


static
RVOID
    os_processes
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    rList procList = NULL;
    rSequence proc = NULL;
    rList mods = NULL;
    processLibProcEntry* entries = NULL;
    RU32 entryIndex = 0;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) &&
        rSequence_addTIMESTAMP( event, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() ) &&
        NULL != ( procList = rList_new( RP_TAGS_PROCESS, RPCM_SEQUENCE ) ) &&
        rSequence_addLIST( event, RP_TAGS_PROCESSES, procList ) )
    {
        entries = processLib_getProcessEntries( TRUE );

        while( NULL != entries && 0 != entries[ entryIndex ].pid )
        {
            if( NULL != ( proc = processLib_getProcessInfo( entries[ entryIndex ].pid ) ) )
            {
                if( NULL != ( mods = processLib_getProcessModules( entries[ entryIndex ].pid ) ) )
                {
                    if( !rSequence_addLIST( proc, RP_TAGS_MODULES, mods ) )
                    {
                        rList_free( mods );
                        mods = NULL;
                    }
                }

                if( !rList_addSEQUENCE( procList, proc ) )
                {
                    rSequence_free( proc );
                    proc = NULL;
                }
            }

            entryIndex++;

            proc = NULL;
            mods = NULL;
        }

        rpal_memory_free( entries );

        notifications_publish( RP_TAGS_NOTIFICATION_OS_PROCESSES_REP, event );
    }
    else
    {
        rList_free( procList );
    }
}

static
RVOID
    os_autoruns
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    rList autoruns = NULL;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) )
    {
        if( rpal_memory_isValid( event ) &&
            NULL != ( autoruns = rList_new( RP_TAGS_AUTORUN, RPCM_SEQUENCE ) ) )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            if( !_getWindowsAutoruns( autoruns ) )
            {
                rpal_debug_warning( "error getting windows autoruns" );
            }
            _enhanceAutorunsWithHashes( autoruns );
#endif

            if( !rSequence_addLIST( event, RP_TAGS_AUTORUNS, autoruns ) )
            {
                rList_free( autoruns );
                autoruns = NULL;
            }

            notifications_publish( RP_TAGS_NOTIFICATION_OS_AUTORUNS_REP, event );
        }
    }
}


static
RPVOID
    allOsSnapshots
    (
        rEvent isTimeToStop,
        RPVOID ctx
    )
{
    rpcm_tag events[] = { RP_TAGS_NOTIFICATION_OS_AUTORUNS_REQ,
                          RP_TAGS_NOTIFICATION_OS_DRIVERS_REQ,
                          RP_TAGS_NOTIFICATION_OS_PROCESSES_REQ,
                          RP_TAGS_NOTIFICATION_OS_SERVICES_REQ };
    RU32 i = 0;
    rSequence dummy = NULL;

    UNREFERENCED_PARAMETER( ctx );

    if( NULL != ( dummy = rSequence_new() ) )
    {
        while( !rEvent_wait( isTimeToStop, MSEC_FROM_SEC( 60 ) ) &&
               rpal_memory_isValid( isTimeToStop ) &&
               i < ARRAY_N_ELEM( events ) )
        {
            notifications_publish( events[ i ], dummy );
            i++;
        }

        rSequence_free( dummy );
    }

    return NULL;
}

static
RVOID
    os_kill_process
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    RU32 pid = 0;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) )
    {
        if( rSequence_getRU32( event, RP_TAGS_PROCESS_ID, &pid ) )
        {
            if( processLib_killProcess( pid ) )
            {
                rSequence_addRU32( event, RP_TAGS_ERROR, RPAL_ERROR_SUCCESS );
            }
            else
            {
                rSequence_addRU32( event, RP_TAGS_ERROR, rpal_error_getLast() );
            }

            rSequence_addTIMESTAMP( event, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );
            notifications_publish( RP_TAGS_NOTIFICATION_OS_KILL_PROCESS_REP, event );
        }
    }
}


RBOOL
    collector_11_init
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;
    RU64 timeDelta = 0;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        if( notifications_subscribe( RP_TAGS_NOTIFICATION_OS_SERVICES_REQ, NULL, 0, NULL, os_services ) &&
            notifications_subscribe( RP_TAGS_NOTIFICATION_OS_DRIVERS_REQ, NULL, 0, NULL, os_drivers ) &&
            notifications_subscribe( RP_TAGS_NOTIFICATION_OS_PROCESSES_REQ, NULL, 0, NULL, os_processes ) &&
            notifications_subscribe( RP_TAGS_NOTIFICATION_OS_AUTORUNS_REQ, NULL, 0, NULL, os_autoruns ) &&
            notifications_subscribe( RP_TAGS_NOTIFICATION_OS_KILL_PROCESS_REQ, NULL, 0, NULL, os_kill_process ) )
        {
            isSuccess = TRUE;

            if( rpal_memory_isValid( config ) &&
                rSequence_getTIMEDELTA( config, RP_TAGS_TIMEDELTA, &timeDelta ) )
            {
                if( !rThreadPool_scheduleRecurring( hbsState->hThreadPool, timeDelta, allOsSnapshots, NULL, TRUE ) )
                {
                    isSuccess = FALSE;
                }
            }
        }
        else
        {
            notifications_unsubscribe( RP_TAGS_NOTIFICATION_OS_SERVICES_REQ, NULL, os_services );
            notifications_unsubscribe( RP_TAGS_NOTIFICATION_OS_DRIVERS_REQ, NULL, os_drivers );
            notifications_unsubscribe( RP_TAGS_NOTIFICATION_OS_PROCESSES_REQ, NULL, os_processes );
            notifications_unsubscribe( RP_TAGS_NOTIFICATION_OS_AUTORUNS_REQ, NULL, os_autoruns );
        }
    }

    return isSuccess;
}

RBOOL
    collector_11_cleanup
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_OS_SERVICES_REQ, NULL, os_services );
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_OS_DRIVERS_REQ, NULL, os_drivers );
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_OS_PROCESSES_REQ, NULL, os_processes );
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_OS_AUTORUNS_REQ, NULL, os_autoruns );
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_OS_KILL_PROCESS_REQ, NULL, os_kill_process );

        isSuccess = TRUE;
    }

    return isSuccess;
}
