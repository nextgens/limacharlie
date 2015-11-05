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

#include <rpal/rpal_file.h>

#define RPAL_FILE_ID     6

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#elif defined( RPAL_PLATFORM_WINDOWS )
    #include <shobjidl.h>
    #include <shlguid.h>
    #include <strsafe.h>
#endif

typedef struct
{
    rStack stack;
    RPWCHAR dirExp;
    RPWCHAR* fileExp;
    RU32 nMaxDepth;

} _rDirCrawl, *_prDirCrawl;


typedef struct
{
#ifdef RPAL_PLATFORM_WINDOWS
    HANDLE handle;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    DIR* handle;
#endif
    RPWCHAR dirPath;
} _rDir;

typedef struct
{
#ifdef RPAL_PLATFORM_WINDOWS
    HANDLE handle;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    FILE* handle;
#endif
} _rFile;


typedef struct
{
#ifdef RPAL_PLATFORM_WINDOWS
    HANDLE hDir;
    RU8 changes[ 64 * 1024 ];
    FILE_NOTIFY_INFORMATION* curChange;
    HANDLE hChange;
    RBOOL includeSubDirs;
    RU32 flags;
    RWCHAR tmpTerminator;
    RPWCHAR pTerminator;
    RBOOL isPending;
    OVERLAPPED oChange;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPAL_PLATFORM_TODO(Directory change notification... might use inotify)
#endif
} _rDirWatch;


RBOOL
    rpal_file_delete
    (
        RPCHAR filePath,
        RBOOL isSafeDelete
    )
{
    RBOOL isDeleted = FALSE;

    RPWCHAR wPath = NULL;

    if( NULL != filePath )
    {
        wPath = rpal_string_atow( filePath );

        if( NULL != wPath )
        {
            isDeleted = rpal_file_deletew( wPath, isSafeDelete );

            rpal_memory_free( wPath );
        }
    }

    return isDeleted;
}

RBOOL
    rpal_file_deletew
    (
        RPWCHAR filePath,
        RBOOL isSafeDelete
    )
{
    RBOOL isDeleted = FALSE;

    RPWCHAR tmpPath = NULL;

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPCHAR localFile = NULL;
#endif

    if( NULL != filePath )
    {
        if( rpal_string_expandw( filePath, &tmpPath ) )
        {
            if( isSafeDelete )
            {
                // TODO: overwrite several times with random data
            }

#ifdef RPAL_PLATFORM_WINDOWS
            if( DeleteFileW( tmpPath ) )
            {
                isDeleted = TRUE;
            }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
            if( NULL != ( localFile = rpal_string_wtoa( tmpPath ) ) )
            {
                if( 0 == unlink( localFile ) )
                {
                    isDeleted = TRUE;
                }
                rpal_memory_free( localFile );
            }
#endif
            rpal_memory_free( tmpPath );
        }
    }

    return isDeleted;
}

RBOOL
    rpal_file_move
    (
        RPCHAR srcFilePath,
        RPCHAR dstFilePath
    )
{
    RBOOL isMoved = FALSE;

    RPWCHAR wSrcPath = NULL;
    RPWCHAR wDstPath = NULL;

    if( NULL != srcFilePath && NULL != dstFilePath )
    {
        if ( NULL != ( wSrcPath = rpal_string_atow( srcFilePath ) ) )
        {
            if ( NULL != ( wDstPath = rpal_string_atow( dstFilePath ) ) ) 
            {
                isMoved = rpal_file_movew( wSrcPath, wDstPath );
                rpal_memory_free( wDstPath );
            }
            rpal_memory_free( wSrcPath );
        }
    }

    return isMoved;
}


RBOOL
    rpal_file_movew
    (
        RPWCHAR srcFilePath,
        RPWCHAR dstFilePath
    )
{
    RBOOL isMoved = FALSE;

    RPWCHAR tmpPath1 = NULL;
    RPWCHAR tmpPath2 = NULL;

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPCHAR localFile1 = NULL;
    RPCHAR localFile2 = NULL;
#endif

    if( NULL != srcFilePath && NULL != dstFilePath )
    {
        if( rpal_string_expandw( srcFilePath, &tmpPath1 ) && rpal_string_expandw( dstFilePath, &tmpPath2 ) )
        {
            
#ifdef RPAL_PLATFORM_WINDOWS
            if( MoveFileW( tmpPath1, tmpPath2 ) )
            {
                isMoved = TRUE;
            }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
            if( NULL != ( localFile1 = rpal_string_wtoa( tmpPath1 ) ) && NULL != ( localFile2 = rpal_string_wtoa( tmpPath2 ) ) )
            {
                if( 0 == rename( localFile1, localFile2 ) )
                {
                    isMoved = TRUE;
                }
                rpal_memory_free( localFile1 );
                rpal_memory_free( localFile2 );
            }
#endif
            rpal_memory_free( tmpPath1 );
            rpal_memory_free( tmpPath2 );
        }
    }

    return isMoved;
}

RBOOL
    rpal_file_getInfo
    (
        RPCHAR filePath,
        rFileInfo* pFileInfo
    )
{
    RBOOL isSuccess = FALSE;

    RPWCHAR wFilePath = NULL;

    if( NULL != filePath && NULL != pFileInfo )
    {
        if ( NULL != ( wFilePath = rpal_string_atow( filePath ) ) )
        {
            isSuccess = rpal_file_getInfow( wFilePath, pFileInfo );
            rpal_memory_free( wFilePath );
        }
    }

    return isSuccess;
}

RBOOL
    rpal_file_getInfow
    (
        RPWCHAR filePath,
        rFileInfo* pFileInfo
    )
{
    RBOOL isSuccess = FALSE;
    
    RPWCHAR expFilePath = NULL;

#ifdef RPAL_PLATFORM_WINDOWS
    WIN32_FIND_DATAW findData = {0};
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPCHAR asciiPath = NULL;
    struct stat fileInfo = {0};
    RPWCHAR widePath = NULL;
#endif

    if ( NULL != filePath && NULL != pFileInfo )
    {
        if( rpal_string_expandw( filePath, &expFilePath ) )
        {
#ifdef RPAL_PLATFORM_WINDOWS

            if( INVALID_HANDLE_VALUE == FindFirstFileW( expFilePath, &findData ) )
            {
                return FALSE;
            }
            else
            {
                pFileInfo->attributes = 0;

                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY );
                }
                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_HIDDEN );
                }
                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_ATTRIBUTE_READONLY ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_READONLY );
                }
                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_ATTRIBUTE_SYSTEM ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_SYSTEM );
                }
                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_ATTRIBUTE_TEMPORARY ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_TEMP );
                }

                pFileInfo->creationTime = rpal_winFileTimeToTs( findData.ftCreationTime );
                pFileInfo->lastAccessTime = rpal_winFileTimeToTs( findData.ftLastAccessTime );
                pFileInfo->modificationTime = rpal_winFileTimeToTs( findData.ftLastWriteTime );

                pFileInfo->size = ( (RU64)findData.nFileSizeHigh << 32 ) | findData.nFileSizeLow;

                rpal_memory_zero( pFileInfo->filePath, sizeof( pFileInfo->filePath ) );

                if( RPAL_MAX_PATH > rpal_string_strlenw( expFilePath ) && RPAL_MAX_PATH > rpal_string_strlenw( findData.cFileName ) &&
                    NULL != rpal_string_strcpyw( pFileInfo->filePath, expFilePath ) )
                {
                    isSuccess = TRUE;
                }
            }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
            rpal_memory_zero( pFileInfo->filePath, sizeof( pFileInfo->filePath ) );

            if( NULL != ( asciiPath = rpal_string_wtoa( expFilePath ) ) )
            {
                if( RPAL_MAX_PATH > rpal_string_strlen( asciiPath ) )
                {
                    if( NULL != ( widePath = rpal_string_atow( asciiPath )  ) )
                    {
                        if( NULL != rpal_string_strcpyw( pFileInfo->filePath, widePath ) )
                        {
                            isSuccess = TRUE;
                        }

                        rpal_memory_free( widePath );
                    }
                }
            }

            if( isSuccess )
            {
                pFileInfo->attributes = 0;
                pFileInfo->creationTime = 0;
                pFileInfo->lastAccessTime = 0;
                pFileInfo->modificationTime = 0;
                pFileInfo->size = 0;

                if( 0 == stat( asciiPath, &fileInfo ) )
                {
                    if( S_ISDIR( fileInfo.st_mode ) )
                    {
                        ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY );
                    }

                    pFileInfo->creationTime = ( (RU64) fileInfo.st_ctime );
                    pFileInfo->lastAccessTime = ( (RU64) fileInfo.st_atime );
                    pFileInfo->modificationTime = ( (RU64) fileInfo.st_mtime );

                    pFileInfo->size = ( (RU64) fileInfo.st_size );
                }
            }

            rpal_memory_free( asciiPath );
#endif
            rpal_memory_free( expFilePath );
        }
    }

    return isSuccess;
}

RBOOL
    rpal_file_read
    (
        RPCHAR filePath,
        RPVOID* pBuffer,
        RU32* pBufferSize,
        RBOOL isAvoidTimestamps
    )
{
    RBOOL isSuccess = FALSE;

    RPWCHAR wPath = NULL;

    if( NULL != filePath )
    {
        wPath = rpal_string_atow( filePath );

        if( NULL != wPath )
        {
            isSuccess = rpal_file_readw( wPath, pBuffer, pBufferSize, isAvoidTimestamps );

            rpal_memory_free( wPath );
        }
    }

    return isSuccess;
}

RBOOL
    rpal_file_readw
    (
        RPWCHAR filePath,
        RPVOID* pBuffer,
        RU32* pBufferSize,
        RBOOL isAvoidTimestamps
    )
{
    RBOOL isSuccess = FALSE;

    RPWCHAR tmpPath = NULL;
    RPVOID tmpFile = NULL;
    RU32 fileSize = 0;

#ifdef RPAL_PLATFORM_WINDOWS
    HANDLE hFile = NULL;
    RU32 flags = 0;
    RU32 access = GENERIC_READ;
    RU32 read = 0;
    FILETIME disableFileTime = { (DWORD)(-1), (DWORD)(-1) };

    flags = FILE_FLAG_SEQUENTIAL_SCAN;
    if( isAvoidTimestamps )
    {
        flags |= FILE_FLAG_BACKUP_SEMANTICS;
        access |= FILE_WRITE_ATTRIBUTES;
    }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    FILE* hFile = NULL;
    RPCHAR localFile = NULL;
    RU8 localBuffer[ 1024 ] = {0};
    RU32 localRead = 0;
#endif

    if( NULL != filePath &&
        NULL != pBuffer &&
        NULL != pBufferSize )
    {
        if( rpal_string_expandw( filePath, &tmpPath ) )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            hFile = CreateFileW( tmpPath, access, FILE_SHARE_READ, NULL, OPEN_EXISTING, flags, NULL );

            if( INVALID_HANDLE_VALUE != hFile )
            {
                if( isAvoidTimestamps )
                {
                    SetFileTime( hFile, NULL, &disableFileTime, NULL );
                }

                fileSize = GetFileSize( hFile, NULL );

                if( 0 != fileSize )
                {
                    tmpFile = rpal_memory_alloc( fileSize );

                    if( rpal_memory_isValid( tmpFile ) )
                    {
                        if( ReadFile( hFile, tmpFile, fileSize, (LPDWORD)&read, NULL ) &&
                            fileSize == read )
                        {
                            isSuccess = TRUE;

                            *pBuffer = tmpFile;
                            *pBufferSize = fileSize;
                        }
                        else
                        {
                            rpal_memory_free( tmpFile );
                        }
                    }
                }
                
                CloseHandle( hFile );
            }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
            if( NULL != ( localFile = rpal_string_wtoa( tmpPath ) ) )
            {
                if( NULL != ( hFile = fopen( localFile, "r" ) ) )
                {
                    // We get the file size as we read since in nix some special files (like in /proc)
                    // will return a file size of 0 if we use the fseek and ftell method.
                    do
                    {
                        localRead = (RU32)fread( localBuffer, sizeof( RU8 ), sizeof( localBuffer ), hFile );
                        
                        if( 0 != localRead )
                        {
                            if( NULL != ( tmpFile = rpal_memory_realloc( tmpFile, fileSize + localRead ) ) )
                            {
                                rpal_memory_memcpy( (RPU8)tmpFile + fileSize, localBuffer, localRead );
                            }
                            
                            fileSize += localRead;
                        }
                    }
                    while( localRead == sizeof( localBuffer ) );
                    
                    if( NULL != tmpFile )
                    {
                        isSuccess = TRUE;
                        *pBuffer = tmpFile;
                        *pBufferSize = fileSize;
                        rpal_memory_zero( localBuffer, sizeof( localBuffer ) );
                    }
                    
                    fclose( hFile );
                }
                
                rpal_memory_free( localFile );
            }
#endif
            rpal_memory_free( tmpPath );
        }
    }

    return isSuccess;
}


RU32
    rpal_file_getSize
    (
        RPCHAR filePath,
        RBOOL isAvoidTimestamps
    )
{
    RBOOL isSuccess = FALSE;

    RPWCHAR wPath = NULL;

    if( NULL != filePath )
    {
        wPath = rpal_string_atow( filePath );

        if( NULL != wPath )
        {
            isSuccess = rpal_file_getSizew( wPath, isAvoidTimestamps );

            rpal_memory_free( wPath );
        }
    }

    return isSuccess;
}


RU32
    rpal_file_getSizew
    (
        RPWCHAR filePath,
        RBOOL isAvoidTimestamps
    )
{
    RU32 size = (RU32)(-1);

    RPWCHAR tmpPath = NULL;

#ifdef RPAL_PLATFORM_WINDOWS
    RU32 flags = 0;
    RU32 access = GENERIC_READ;
    HANDLE hFile = NULL;
    FILETIME disableFileTime = { (DWORD)(-1), (DWORD)(-1) };

    flags = 0;
    if( isAvoidTimestamps )
    {
        flags |= FILE_FLAG_BACKUP_SEMANTICS;
        access |= FILE_WRITE_ATTRIBUTES;
    }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPCHAR localFile = NULL;
    FILE* hFile = NULL;
#endif

    if( NULL != filePath )
    {
        if( rpal_string_expandw( filePath, &tmpPath ) )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            hFile = CreateFileW( tmpPath, access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, flags, NULL );
            
            if( INVALID_HANDLE_VALUE != hFile )
            {
                if( isAvoidTimestamps )
                {
                    SetFileTime( hFile, NULL, &disableFileTime, NULL );
                }

                size = GetFileSize( hFile, NULL );
                
                CloseHandle( hFile );
            }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
            if( NULL != ( localFile = rpal_string_wtoa( tmpPath ) ) )
            {
                if( NULL != ( hFile = fopen( localFile, "r" ) ) )
                {
                    if( 0 == fseek( hFile, 0, SEEK_END ) )
                    {
                        if( (-1) != ( size = (RU32)ftell( hFile ) ) )
                        {
                            // Success
                        }
                    }
                    
                    fclose( hFile );
                }
                
                rpal_memory_free( localFile );
            }
#endif
            rpal_memory_free( tmpPath );
        }
    }

    return size;
}


RBOOL
    rpal_file_write
    (
        RPCHAR filePath,
        RPVOID buffer,
        RU32 bufferSize,
        RBOOL isOverwrite
    )
{
    RBOOL isSuccess = FALSE;

    RPWCHAR wPath = NULL;

    if( NULL != filePath )
    {
        wPath = rpal_string_atow( filePath );

        if( NULL != wPath )
        {
            isSuccess = rpal_file_writew( wPath, buffer, bufferSize, isOverwrite );

            rpal_memory_free( wPath );
        }
    }

    return isSuccess;
}

RBOOL
    rpal_file_writew
    (
        RPWCHAR filePath,
        RPVOID buffer,
        RU32 bufferSize,
        RBOOL isOverwrite
    )
{
    RBOOL isSuccess = FALSE;

    RPWCHAR tmpPath = NULL;

#ifdef RPAL_PLATFORM_WINDOWS
    RU32 flags = 0;
    RU32 written = 0;
    HANDLE hFile = NULL;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPCHAR localFile = NULL;
    FILE* hFile = NULL;
#endif

    if( NULL != filePath &&
        NULL != buffer &&
        0 != bufferSize )
    {
        if( rpal_string_expandw( filePath, &tmpPath ) )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            hFile = CreateFileW( tmpPath, 
                                GENERIC_WRITE, 
                                0, 
                                NULL, 
                                isOverwrite ? CREATE_ALWAYS : CREATE_NEW, 
                                flags, 
                                NULL );

            if( INVALID_HANDLE_VALUE != hFile )
            {
                if( WriteFile( hFile, buffer, bufferSize, (LPDWORD)&written, NULL ) &&
                    bufferSize == written )
                {
                    isSuccess = TRUE;
                }

                CloseHandle( hFile );
            }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
            if( NULL != ( localFile = rpal_string_wtoa( tmpPath ) ) )
            {
                if( isOverwrite ||
                    NULL == ( hFile = fopen( localFile, "r" ) ) )
                {
                    if( NULL != ( hFile = fopen( localFile, "w" ) ) )
                    {
                        if( 1 == fwrite( buffer, bufferSize, 1, hFile ) )
                        {
                            isSuccess = TRUE;
                        }
                        
                        fclose( hFile );
                    }
                }
                else
                {
                    // File already exists and we're not to overwrite...
                    fclose( hFile );
                }
                
                rpal_memory_free( localFile );
            }
#endif
            rpal_memory_free( tmpPath );
        }
    }

    return isSuccess;
}



RBOOL
    rpal_file_pathToLocalSepW
    (
        RPWCHAR path
    )
{
    RBOOL isSuccess = FALSE;
    RWCHAR search = 0;
    RWCHAR replace = 0;
    RU32 i = 0;

    if( NULL != path )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        search = _WCH('/');
        replace = _WCH('\\');
#else
        search = _WCH('\\');
        replace = _WCH('/');
#endif

        for( i = 0; i < rpal_string_strlenw( path ); i++ )
        {
            if( search == path[ i ] )
            {
                path[ i ] = replace;
            }
        }

        isSuccess = TRUE;
    }

    return isSuccess;
}

RBOOL
    rpal_file_getLinkDest
    (
        RPWCHAR linkPath,
        RPWCHAR* pDestination
    )
{
    RBOOL isSuccess = FALSE;
    
    #ifdef RPAL_PLATFORM_WINDOWS
    HRESULT hres = 0;
    REFCLSID clsid_shelllink = &CLSID_ShellLink;
    REFIID ref_ishelllink = &IID_IShellLinkW;
    REFIID ref_ipersisfile = &IID_IPersistFile;
    IShellLinkW* psl = NULL;
    IPersistFile* ppf = NULL;
    WCHAR szGotPath[ MAX_PATH ] = {0};

    UNREFERENCED_PARAMETER( linkPath );
    UNREFERENCED_PARAMETER( pDestination );
    
    hres = CoInitialize( NULL );
    if( S_OK == hres || 
        S_FALSE == hres || 
        RPC_E_CHANGED_MODE == hres )
    {
        hres = CoCreateInstance( clsid_shelllink, NULL, CLSCTX_INPROC_SERVER, ref_ishelllink, (LPVOID*)&psl );
        if( SUCCEEDED( hres ) )
        {
            hres = psl->lpVtbl->QueryInterface( psl, ref_ipersisfile, (void**)&ppf );
        
            if( SUCCEEDED( hres ) ) 
            {
                hres = ppf->lpVtbl->Load( ppf, linkPath, STGM_READ );
            
                if( SUCCEEDED( hres ) )
                {
                    hres = psl->lpVtbl->GetPath( psl, szGotPath, MAX_PATH, NULL, 0 );

                    if( SUCCEEDED( hres ) )
                    {
                        if( NULL != ( *pDestination = rpal_string_strdupw( szGotPath ) ) )
                        {
                            isSuccess = TRUE;
                        }
                    }
                }

                ppf->lpVtbl->Release( ppf );
            }

            psl->lpVtbl->Release( psl );
        }

        CoUninitialize();
    }
    #elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    if( NULL != linkPath &&
        NULL != pDestination )
    {
        RS32 size = 0;
        RPCHAR path = NULL;
        RCHAR tmp[ RPAL_MAX_PATH + 1 ] = {0};
        if( NULL != ( path = rpal_string_wtoa( linkPath ) ) )
        {
            size = (RU32)readlink( path, (RPCHAR)&tmp, sizeof( tmp ) - 1 );
            if( -1 != size
               && ( sizeof( tmp ) - 1 ) >= size )
            {
                if( NULL != ( *pDestination = rpal_string_atow( (RPCHAR)&tmp ) ) )
                {
                    isSuccess = TRUE;
                }
            }
            
            rpal_memory_free( path );
        }
    }
    #endif
    
    return isSuccess;
}

RPWCHAR
    rpal_file_filePathToFileName
    (
        RPWCHAR filePath
    )
{
    RPWCHAR fileName = NULL;

    RU32 len = 0;

    if( NULL != filePath )
    {
        len = rpal_string_strlenw( filePath );
        while( 0 != len )
        {
            len--;

#ifdef RPAL_PLATFORM_WINDOWS
            if( _WCH( '\\' ) == filePath[ len ] ||
                _WCH( '/' ) == filePath[ len ] )
#else
            if( _WCH( '/' ) == filePath[ len ] )
#endif
            {
                fileName = &( filePath[ len + 1 ] );
                break;
            }
        }

        if( NULL == fileName )
        {
            fileName = filePath;
        }
    }

    return fileName;
}

static
rDirCrawl
    _newCrawlDir
    (
        RPWCHAR rootExpr,
        RPWCHAR fileExpr[],
        RU32 nMaxDepth
    )
{
    _prDirCrawl pCrawl = NULL;

    if( NULL != rootExpr &&
        NULL != fileExpr )
    {
        pCrawl = rpal_memory_alloc( sizeof( _rDirCrawl ) );

        if( NULL != pCrawl )
        {
            pCrawl->stack = rStack_new( sizeof( rFileInfo ) );

            if( NULL != pCrawl->stack )
            {
                pCrawl->dirExp = NULL;
                pCrawl->fileExp = NULL;
                pCrawl->nMaxDepth = nMaxDepth;
                
                rpal_string_expandw( rootExpr, &(pCrawl->dirExp) );
                pCrawl->fileExp = fileExpr;

                if( NULL == pCrawl->dirExp ||
                    NULL == pCrawl->fileExp )
                {
                    rpal_memory_free( pCrawl->dirExp );
                    rStack_free( pCrawl->stack, NULL );
                    rpal_memory_free( pCrawl );
                    pCrawl = NULL;
                }
                else
                {
                    rpal_file_pathToLocalSepW( pCrawl->dirExp );
                }
            }
            else
            {
                rpal_memory_free( pCrawl );
                pCrawl = NULL;
            }
        }
    }

    return pCrawl;
}

RBOOL
    _strHasWildcards
    (
        RPWCHAR str
    )
{
    RBOOL isWild = FALSE;

    RU32 i = 0;
    RU32 len = 0;

    len = rpal_string_strlenw( str );

    for( i = 0; i < len; i++ )
    {
        if( _WCH('*') == str[ i ] ||
            _WCH('?') == str[ i ] )
        {
            isWild = TRUE;
            break;
        }
    }

    return isWild;
}

RBOOL
    _isFileInfoInCrawl
    (
        RPWCHAR dirExp,
        RPWCHAR fileExp[],
        RU32 nMaxDepth,
        rFileInfo* pInfo,
        RBOOL isPartialOk
    )
{
    RBOOL isIncluded = FALSE;

    RWCHAR sep[] = RPAL_FILE_LOCAL_DIR_SEP;

    RPWCHAR state1 = NULL;
    RPWCHAR state2 = NULL;
    RPWCHAR pPattern = NULL;
    RPWCHAR pPath = NULL;

    RU32 curDepth = 0;

    RPWCHAR* tmpFileExp = NULL;

    if( NULL != dirExp &&
        NULL != fileExp &&
        NULL != pInfo )
    {
        // Start by evaluating the path
        // Temporarily terminate the path part, if this is a directory then 
        // it already is a normal path...
        if( !IS_FLAG_ENABLED( pInfo->attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY ) )
        {
            *( pInfo->fileName - 1 ) = 0;
        }

        // Loop through the pattern and path at the same time
        pPattern = dirExp;
        pPath = pInfo->filePath;

        pPattern = rpal_string_strtokw( pPattern, sep[ 0 ], &state1 );
        pPath = rpal_string_strtokw( pPath, sep[ 0 ], &state2 );

        while( NULL != pPattern &&
               NULL != pPath )
        {
            if( !rpal_string_matchw( pPattern, pPath ) )
            {
                break;
            }

            do
            {
                pPattern = rpal_string_strtokw( NULL, sep[ 0 ], &state1 );
            }
            while( NULL != pPattern && 0 == pPattern[ 0 ] );    // We do this to support path with "//" or "\\"

            do
            {
                pPath = rpal_string_strtokw( NULL, sep[ 0 ], &state2 );
            }
            while( NULL != pPath && 0 == pPath[ 0 ] );
        }

        // The match is valid if the path matches all tokens of
        // the pattern but it can also be longer, as long as it matches.
        if( NULL == pPattern )
        {
            isIncluded = TRUE;
        }

        // If the path ran out first, check if partial match is ok. This is used
        // to match a directory and see if it should be crawled further...
        if( NULL == pPath &&
            isPartialOk )
        {
            isIncluded = TRUE;
        }
        // Unwind the path if necessary
        else if( NULL != pPath )
        {
            // So the path is longer than the criteria, this means we have to 
            // validate whether we are past maxDepth...
            do
            {
                curDepth++;
            }
            while( NULL != rpal_string_strtokw( NULL, sep[ 0 ], &state2 ) );

            if( curDepth > ( nMaxDepth + 1 ) )
            {
                // We are past our depth
                isIncluded = FALSE;
            }
        }

        // Unwind the pattern if necessary
        if( NULL != pPattern )
        {
            while( NULL != rpal_string_strtokw( NULL, sep[ 0 ], &state1 ) ){}
        }

        // Even if this is a dir, it doesn't hurt to overwrite the sep.
        *( pInfo->fileName - 1 ) = sep[ 0 ];

        // If the path has matched so far
        if( isIncluded )
        {
            isIncluded = FALSE;

            // If this is not a check for partial data, we much always validate the file name.
            if( !isPartialOk )
            {
                // If this is not a directory, we much check to make sure the file matches.
                tmpFileExp = fileExp;
                while( NULL != *tmpFileExp )
                {
                    if( rpal_string_matchw( *tmpFileExp, pInfo->fileName ) )
                    {
                        isIncluded = TRUE;
                        break;
                    }

                    tmpFileExp++;
                }
            }
            else
            {
                isIncluded = TRUE;
            }
        }
    }

    return isIncluded;
}

RVOID
    _fixFileInfoAfterPop
    (
        rFileInfo* pInfo
    )
{
    RU32 len = 0;
    RWCHAR sep[] = RPAL_FILE_LOCAL_DIR_SEP;

    if( NULL != pInfo )
    {
        len = rpal_string_strlenw( pInfo->filePath );

        if( 0 != len )
        {
            len--;

            while( 0 != len )
            {
                if( sep[ 0 ] == pInfo->filePath[ len ] )
                {
                    pInfo->fileName = &pInfo->filePath[ len + 1 ];
                    break;
                }

                len--;
            }
        }
    }
}


rDirCrawl
    rpal_file_crawlStart
    (
        RPWCHAR rootExpr,
        RPWCHAR fileExpr[],
        RU32 nMaxDepth
    )
{
    _prDirCrawl pCrawl = NULL;
    RPWCHAR staticRoot = NULL;
    RPWCHAR tmpStr = NULL;
    RPWCHAR state = NULL;
    RPWCHAR sep = RPAL_FILE_LOCAL_DIR_SEP;
    rDir hDir = NULL;
    rFileInfo info = {0};

    pCrawl = _newCrawlDir( rootExpr, fileExpr, nMaxDepth );
    
    if( NULL != pCrawl )
    {
        pCrawl->nMaxDepth = nMaxDepth;
        
        if( NULL != ( tmpStr = rpal_string_strtokw( pCrawl->dirExp, sep[ 0 ], &state ) ) )
        {
            do
            {
                if( !_strHasWildcards( tmpStr ) )
                {
                    staticRoot = rpal_string_strcatExW( staticRoot, tmpStr );
                    staticRoot = rpal_string_strcatExW( staticRoot, sep );
                }
                else
                {
                    // Unwind the tokens to restore the string
                    while( NULL != ( tmpStr = rpal_string_strtokw( NULL, sep[ 0 ], &state ) ) );
                    break;
                }
            }
            while( NULL != ( tmpStr = rpal_string_strtokw( NULL, sep[ 0 ], &state ) ) );
        }

        if( NULL != staticRoot )
        {
            staticRoot[ rpal_string_strlenw( staticRoot ) - 1 ] = 0;
            
            if( rDir_open( staticRoot, &hDir ) )
            {
                while( rDir_next( hDir, &info ) )
                {
                    if( _isFileInfoInCrawl( pCrawl->dirExp, 
                                            pCrawl->fileExp, 
                                            pCrawl->nMaxDepth, 
                                            &info,
                                            IS_FLAG_ENABLED( info.attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY ) ) ||
                        ( IS_FLAG_ENABLED( info.attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY ) &&  // If a directory is not
                          _isFileInfoInCrawl( pCrawl->dirExp,                                 // to be crawled, it may
                                              pCrawl->fileExp,                                // still be a perfect match.
                                              pCrawl->nMaxDepth, 
                                              &info, 
                                              FALSE ) ) )
                    {
                        // Match dir against dir and file again file???????
                        if( !rStack_push( pCrawl->stack, &info ) )
                        {
                            rpal_file_crawlStop( pCrawl );
                            pCrawl = NULL;
                            break;
                        }
                    }
                }

                rDir_close( hDir );
            }

            rpal_memory_free( staticRoot );
        }
        else
        {
            rpal_file_crawlStop( pCrawl );
            pCrawl = NULL;
        }
    }

    return pCrawl;
}


RBOOL
    rpal_file_crawlNextFile
    (
        rDirCrawl hCrawl,
        rFileInfo* pFileInfo
    )
{
    RBOOL isSuccess = FALSE;

    _prDirCrawl pCrawl = (_prDirCrawl)hCrawl;
    rFileInfo info = {0};
    rDir tmpDir = NULL;
    
    if( NULL != pCrawl )
    {
        while( !isSuccess &&
               rStack_pop( pCrawl->stack, &info ) )
        {
            // The ptr in the info is a local ptr, so it probably changes when we 
            // moved the memory so we have to "re-base" it using the path.
            _fixFileInfoAfterPop( &info );

            // If it's a directory, we will report it but drill down
            if( IS_FLAG_ENABLED( info.attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY ) )
            {
                // Dirs on the stack do NOT necessarily match the criteria. We may have pushed
                // them simply because they PARTIALLY matched the criteria and therefore we needed
                // them to keep drilling down...
                if( _isFileInfoInCrawl( pCrawl->dirExp, pCrawl->fileExp, pCrawl->nMaxDepth, &info, FALSE ) )
                {
                    *pFileInfo = info;
                    _fixFileInfoAfterPop( pFileInfo );
                    isSuccess = TRUE;
                }

                // This is a shortcut if depth is 0, we will never want to drill down further
                if( 0 != pCrawl->nMaxDepth )
                {
                    if( rDir_open( info.filePath, &tmpDir ) )
                    {
                        while( rDir_next( tmpDir, &info ) )
                        {
                            if( _isFileInfoInCrawl( pCrawl->dirExp, 
                                                    pCrawl->fileExp, 
                                                    pCrawl->nMaxDepth, 
                                                    &info, 
                                                    IS_FLAG_ENABLED( info.attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY ) ) ||
                                ( IS_FLAG_ENABLED( info.attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY ) &&  // If a directory is not
                                  _isFileInfoInCrawl( pCrawl->dirExp,                                   // to be crawled, it may
                                                      pCrawl->fileExp,                                  // still be a perfect match.
                                                      pCrawl->nMaxDepth, 
                                                      &info, 
                                                      FALSE ) ) )
                            {
                                if( !rStack_push( pCrawl->stack, &info ) )
                                {
                                    // This is bad, but not much we can do
                                    isSuccess = FALSE;
                                    break;
                                }
                            }
                        }

                        rDir_close( tmpDir );
                    }
                }
            }
            else
            {
                // Files on the stack are always matching since we check before
                // pushing them onto the stack.
                *pFileInfo = info;
                _fixFileInfoAfterPop( pFileInfo );
                isSuccess = TRUE;
            }
        }
    }

    return isSuccess;
}


#ifdef RPAL_PLATFORM_WINDOWS
static
RBOOL
    _freeStringWrapper
    (
        RPWCHAR str
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != str )
    {
        rpal_memory_free( str );
        isSuccess = TRUE;
    }

    return isSuccess;
}
#endif


RVOID
    rpal_file_crawlStop
    (
        rDirCrawl hDirCrawl
    )
{
    _prDirCrawl pCrawl = (_prDirCrawl)hDirCrawl;

    if( NULL != hDirCrawl )
    {
        rStack_free( pCrawl->stack, NULL );
        rpal_memory_free( pCrawl->dirExp );
        rpal_memory_free( pCrawl );
    }
}


RBOOL
    rDir_open
    (
        RPWCHAR dirPath,
        rDir* phDir
    )
{
    RBOOL isSuccess = FALSE;
    _rDir* dir = NULL;
    RWCHAR sep[] = RPAL_FILE_LOCAL_DIR_SEP;
    RPWCHAR tmpDir = NULL;
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPCHAR asciiDir = NULL;
#endif

    if( NULL != dirPath &&
        NULL != phDir &&
        0 < rpal_string_strlenw( dirPath ) )
    {
        if( rpal_string_expandw( dirPath, &tmpDir ) )
        {
            dir = rpal_memory_alloc( sizeof( *dir ) );
            
            if( NULL != dir )
            {
                dir->handle = NULL;
                
                if( NULL != ( dir->dirPath = rpal_string_strdupw( tmpDir ) ) )
                {
                    rpal_file_pathToLocalSepW( dir->dirPath );
                    
                    if( sep[ 0 ] != dir->dirPath[ rpal_string_strlenw( dir->dirPath ) - 1  ] )
                    {
                        dir->dirPath = rpal_string_strcatExW( dir->dirPath, sep );
                    }
                    
                    if( NULL != dir->dirPath )
                    {
#ifdef RPAL_PLATFORM_WINDOWS
                        *phDir = dir;
                        isSuccess = TRUE;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                        if( NULL != ( asciiDir = rpal_string_wtoa( dir->dirPath ) ) )
                        {
                            if( NULL != ( dir->handle = opendir( asciiDir ) ) )
                            {
                                *phDir = dir;
                                isSuccess = TRUE;
                            }
                            else
                            {
                                rpal_memory_free( dir->dirPath );
                                rpal_memory_free( dir );
                            }
                            
                            rpal_memory_free( asciiDir );
                        }
#endif
                    }
                    else
                    {
                        rpal_memory_free( dir );
                        dir = NULL;
                    }
                }
                else
                {
                    rpal_memory_free( dir );
                }
            }
            
            rpal_memory_free( tmpDir );
        }
    }

    return isSuccess;
}

RVOID
    rDir_close
    (
        rDir hDir
    )
{
    if( NULL != hDir )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        if( INVALID_HANDLE_VALUE != hDir )
        {
            FindClose( ((_rDir*)hDir)->handle );
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( NULL != hDir )
        {
            closedir( ((_rDir*)hDir)->handle );
        }
#endif
        rpal_memory_free( ((_rDir*)hDir)->dirPath );

        rpal_memory_free( hDir );
    }
}

RBOOL
    rDir_next
    (
        rDir hDir,
        rFileInfo* pFileInfo
    )
{
    RBOOL isSuccess = FALSE;
    _rDir* dir = (_rDir*)hDir;
    RBOOL isDataReady = FALSE;

#ifdef RPAL_PLATFORM_WINDOWS
    WIN32_FIND_DATAW findData = {0};
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    struct dirent *findData = NULL;
    RPCHAR asciiPath = NULL;
    struct stat fileInfo = {0};
    RPWCHAR widePath = NULL;
#endif

    if( NULL != hDir &&
        NULL != pFileInfo )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        if( INVALID_HANDLE_VALUE != dir->handle )
        {
            if( NULL == dir->handle )
            {
                if( _WCH('*') != dir->dirPath[ rpal_string_strlenw( dir->dirPath ) - 1 ] )
                {
                    dir->dirPath = rpal_string_strcatExW( dir->dirPath, _WCH("*") );
                }

                if( INVALID_HANDLE_VALUE == ( dir->handle = FindFirstFileW( dir->dirPath, &findData ) ) )
                {
                    return FALSE;
                }
                else
                {
                    isDataReady = TRUE;
                }

                // We can now remove the trailing *
                dir->dirPath[ rpal_string_strlenw( dir->dirPath ) - 1 ] = 0;
            }

            if( !isDataReady )
            {
                if( !FindNextFileW( dir->handle, &findData ) )
                {
                    return FALSE;
                }
                else
                {
                    isDataReady = TRUE;
                }
            }

            if( isDataReady )
            {
                while( 0 == rpal_string_strcmpw( _WCH("."), findData.cFileName ) ||
                       0 == rpal_string_strcmpw( _WCH(".."), findData.cFileName ) )
                {
                    if( !FindNextFileW( dir->handle, &findData ) )
                    {
                        isDataReady = FALSE;
                        break;
                    }
                }
            }

            if( isDataReady )
            {
                pFileInfo->attributes = 0;

                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY );
                }
                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_HIDDEN );
                }
                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_ATTRIBUTE_READONLY ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_READONLY );
                }
                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_ATTRIBUTE_SYSTEM ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_SYSTEM );
                }
                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_ATTRIBUTE_TEMPORARY ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_TEMP );
                }
                if( IS_FLAG_ENABLED( findData.dwFileAttributes, FILE_EXECUTE ) )
                {
                    ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_EXECUTE );
                }

                pFileInfo->creationTime = rpal_winFileTimeToTs( findData.ftCreationTime );
                pFileInfo->lastAccessTime = rpal_winFileTimeToTs( findData.ftLastAccessTime );
                pFileInfo->modificationTime = rpal_winFileTimeToTs( findData.ftLastWriteTime );

                pFileInfo->size = ( (RU64)findData.nFileSizeHigh << 32 ) | findData.nFileSizeLow;

                rpal_memory_zero( pFileInfo->filePath, sizeof( pFileInfo->filePath ) );

                if( RPAL_MAX_PATH > ( rpal_string_strlenw( dir->dirPath ) + 
                                      rpal_string_strlenw( findData.cFileName ) ) &&
                    NULL != rpal_string_strcatw( pFileInfo->filePath, dir->dirPath ) &&
                    NULL != ( pFileInfo->fileName = ( pFileInfo->filePath + rpal_string_strlenw( pFileInfo->filePath ) ) ) &&
                    NULL != rpal_string_strcatw( pFileInfo->filePath, findData.cFileName ) )
                {
                    isSuccess = TRUE;
                }
            }
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( NULL != hDir )
        {
            if( NULL != dir->handle )
            {
                if( NULL != ( findData = readdir( dir->handle ) ) )
                {
                    isDataReady = TRUE;
                }
            }
            
            if( isDataReady )
            {
                while( 0 == rpal_string_strcmpa( ".", findData->d_name ) ||
                       0 == rpal_string_strcmpa( "..", findData->d_name ) )
                {
                    if( NULL == ( findData = readdir( dir->handle ) ) )
                    {
                        isDataReady = FALSE;
                        break;
                    }
                }
            }
            
            if( isDataReady )
            {
                rpal_memory_zero( pFileInfo->filePath, sizeof( pFileInfo->filePath ) );
                
                if( NULL != ( asciiPath = rpal_string_wtoa( dir->dirPath ) ) )
                {
                    if( NULL != ( asciiPath = rpal_string_strcatExA( asciiPath, findData->d_name ) ) )
                    {
                        if( RPAL_MAX_PATH > rpal_string_strlen( asciiPath ) )
                        {
                            if( NULL != ( widePath = rpal_string_atow( asciiPath )  ) )
                            {
                                if( NULL != rpal_string_strcpyw( pFileInfo->filePath, widePath ) )
                                {
                                    pFileInfo->fileName = pFileInfo->filePath + rpal_string_strlenw( dir->dirPath );
                                    isSuccess = TRUE;
                                }
                                
                                rpal_memory_free( widePath );
                            }
                        }
                    }
                }
                
                if( isSuccess )
                {
                    pFileInfo->attributes = 0;
                    pFileInfo->creationTime = 0;
                    pFileInfo->lastAccessTime = 0;
                    pFileInfo->modificationTime = 0;
                    pFileInfo->size = 0;
                    
                    if( 0 == stat( asciiPath, &fileInfo ) )
                    {
                        if( S_ISDIR( fileInfo.st_mode ) )
                        {
                            ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_DIRECTORY );
                        }
                        if( IS_FLAG_ENABLED( S_IXUSR, fileInfo.st_mode ) )
                        {
                            ENABLE_FLAG( pFileInfo->attributes, RPAL_FILE_ATTRIBUTE_EXECUTE );
                        }
                        
                        pFileInfo->creationTime = ( (RU64) fileInfo.st_ctime );
                        pFileInfo->lastAccessTime = ( (RU64) fileInfo.st_atime );
                        pFileInfo->modificationTime = ( (RU64) fileInfo.st_mtime );
                        
                        pFileInfo->size = ( (RU64) fileInfo.st_size );
                    }
                }
                
                rpal_memory_free( asciiPath );
            }
        }
#endif
    }

    return isSuccess;
}

RBOOL
    rDir_create
    (
        RPWCHAR dirPath
    )
{
    RBOOL isCreated = FALSE;

    RPWCHAR expDir = NULL;

    if( NULL != dirPath )
    {
        if( rpal_string_expandw( dirPath, &expDir ) )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            if( CreateDirectoryW( expDir, NULL ) )
            {
                isCreated = TRUE;
            }
#else
            RPCHAR aDir = NULL;

            if( NULL != ( aDir = rpal_string_wtoa( expDir ) ) )
            {
                if( 0 == mkdir( aDir, 0700 ) )
                {
                    isCreated = TRUE;
                }

                rpal_memory_free( aDir );
            }
#endif
            rpal_memory_free( expDir );
        }
    }

    return isCreated;
}

RBOOL
    rFile_open
    (
        RPWCHAR filePath,
        rFile* phFile,
        RU32 flags
    )
{
    RBOOL isSuccess = FALSE;
    _rFile* hFile = NULL;
    RPWCHAR tmpPath = NULL;
#ifdef RPAL_PLATFORM_WINDOWS
    RU32 osAccessFlags = 0;
    RU32 osCreateFlags = 0;
    RU32 osAttributeFlags = 0;
    FILETIME disableFileTime = { (DWORD)(-1), (DWORD)(-1) };
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPCHAR asciiPath = NULL;
    RPCHAR fileMode = NULL;
    struct stat fileInfo = {0};
#endif

    if( NULL != filePath &&
        NULL != phFile &&
        rpal_string_expandw( filePath, &tmpPath ) &&
        NULL != tmpPath )
    {
        rpal_file_pathToLocalSepW( tmpPath );

        hFile = rpal_memory_alloc( sizeof( *hFile ) );

        if( NULL != hFile )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_READ ) )
            {
                ENABLE_FLAG( osAccessFlags, GENERIC_READ );
            }
            if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_WRITE ) )
            {
                ENABLE_FLAG( osAccessFlags, GENERIC_WRITE );
            }
            if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_EXISTING ) )
            {
                ENABLE_FLAG( osCreateFlags, OPEN_EXISTING );
            }
            if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_NEW ) )
            {
                ENABLE_FLAG( osCreateFlags, CREATE_NEW );
            }
            if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_ALWAYS ) )
            {
                ENABLE_FLAG( osCreateFlags, OPEN_ALWAYS );
            }
            if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_AVOID_TIMESTAMPS ) )
            {
                ENABLE_FLAG( osAttributeFlags, FILE_FLAG_BACKUP_SEMANTICS );
                ENABLE_FLAG( osAccessFlags, FILE_WRITE_ATTRIBUTES );
            }

            hFile->handle = CreateFileW( tmpPath,
                                         osAccessFlags,
                                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         NULL,
                                         osCreateFlags,
                                         osAttributeFlags,
                                         NULL );

            if( INVALID_HANDLE_VALUE == hFile->handle )
            {
                rpal_memory_free( hFile );
                hFile = NULL;
            }
            else
            {
                if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_AVOID_TIMESTAMPS ) )
                {
                    SetFileTime( hFile->handle, NULL, &disableFileTime, &disableFileTime );
                }

                isSuccess = TRUE;
                *phFile = hFile;
            }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
            if( NULL != ( asciiPath = rpal_string_wtoa( filePath ) ) )
            {
                if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_EXISTING ) )
                {
                    // THE FILE MUST EXIST
                    //====================
                    if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_READ ) )
                    {
                        if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_WRITE ) )
                        {
                            fileMode = "r+";
                        }
                        else
                        {
                            fileMode = "r";
                        }
                    }
                    else if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_WRITE ) &&
                             0 == stat( asciiPath, &fileInfo ) )
                    {
                        fileMode = "r+";
                    }
                }
                else if ( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_NEW ) &&
                          0 != stat( asciiPath, &fileInfo ) )
                {
                    // THE FILE CANNOT EXIST
                    //======================
                    if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_READ ) )
                    {
                        if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_WRITE ) )
                        {
                            fileMode = "w+";
                        }
                        else
                        {
                            // Makes no sense to open a NEW file for READING
                            fileMode = "w+";
                        }
                    }
                    else if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_WRITE ) )
                    {
                        fileMode = "w";
                    }
                }
                else if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_ALWAYS ) )
                {
                    if( 0 != stat( asciiPath, &fileInfo ) )
                    {
                        // The file doesn't exist, we create it. None of this is atomic, but
                        // for now it should be enough to get going...
                        if( NULL != ( hFile->handle = fopen( asciiPath, "a+" ) ) )
                        {
                            fclose( hFile->handle );
                        }
                    }

                    // WE DON'T CARE IF IT EXISTS
                    //======================
                    if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_READ ) )
                    {
                        if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_WRITE ) )
                        {
                            fileMode = "r+";
                        }
                        else
                        {
                            fileMode = "r";
                        }
                    }
                    else if( IS_FLAG_ENABLED( flags, RPAL_FILE_OPEN_WRITE ) )
                    {
                        fileMode = "r+";
                    }
                }

                // We only proceed if the file mode made sense
                if( NULL != fileMode )
                {
                    if( NULL != ( hFile->handle = fopen( asciiPath, fileMode ) ) )
                    {
                        isSuccess = TRUE;
                        *phFile = hFile;
                    }
                    else
                    {
                        rpal_memory_free( hFile );
                        hFile = NULL;
                    }
                }
                rpal_memory_free( asciiPath );
            }
#endif
            rpal_memory_free( tmpPath );
        }
    }

    return isSuccess;
}

RVOID
    rFile_close
    (
        rFile hFile
    )
{
    _rFile* pFile = (_rFile*)hFile;

    if( NULL != hFile )
    {
        if( NULL != pFile->handle )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            if( INVALID_HANDLE_VALUE != pFile->handle )
            {
                CloseHandle( pFile->handle );
            }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
            fclose( pFile->handle );
#endif
        }

        rpal_memory_free( pFile );
    }
}

RU64
    rFile_seek
    (
        rFile hFile,
        RU64 offset,
        enum rFileSeek origin
    )
{
    RU64 newPtr = (unsigned)(-1);
    _rFile* pFile = (_rFile*)hFile;
    RU32 method = 0;

    if( NULL != hFile )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        LARGE_INTEGER off;
        LARGE_INTEGER newP;
        
        off.QuadPart = offset;
        newP.QuadPart = 0;

        if( rFileSeek_CUR == origin )
        {
            method = FILE_CURRENT;
        }
        else if( rFileSeek_SET == origin )
        {
            method = FILE_BEGIN;
        }
        else if( rFileSeek_END == origin )
        {
            method = FILE_END;
        }
        
        if( SetFilePointerEx( pFile->handle, off, &newP, method ) )
        {
            newPtr = newP.QuadPart;
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( rFileSeek_CUR == origin )
        {
            method = SEEK_CUR;
        }
        else if( rFileSeek_SET == origin )
        {
            method = SEEK_SET;
        }
        else if( rFileSeek_END == origin )
        {
            method = SEEK_END;
        }
        
        if( 0 == fseek( pFile->handle, offset, method ) )
        {
            newPtr = ftell( pFile->handle );
        }
#endif
    }

    return newPtr;
}

RBOOL
    rFile_read
    (
        rFile hFile,
        RU32 size,
        RPVOID pBuffer
    )
{
    RBOOL isSuccess = FALSE;
    _rFile* pFile = (_rFile*)hFile;

    if( NULL != hFile &&
        NULL != pBuffer &&
        0 != size )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        RU32 read = 0;

        if( ReadFile( pFile->handle, pBuffer, size, (LPDWORD)&read, NULL ) &&
            read == size )
        {
            isSuccess = TRUE;
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( 1 == fread( pBuffer, size, 1, pFile->handle ) )
        {
            isSuccess = TRUE;
        }
#endif
    }

    return isSuccess;
}

RU32
    rFile_readUpTo
    (
        rFile hFile,
        RU32 size,
        RPVOID pBuffer
    )
{
    RU32 read = 0;
    _rFile* pFile = (_rFile*)hFile;

    if( NULL != hFile &&
        NULL != pBuffer &&
        0 != size )
    {
#ifdef RPAL_PLATFORM_WINDOWS

        ReadFile( pFile->handle, pBuffer, size, (LPDWORD)&read, NULL );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        read = (RU32)fread( pBuffer, 1, size, pFile->handle );
        
#endif
    }

    return read;
}

RBOOL
    rFile_write
    (
        rFile hFile,
        RU32 size,
        RPVOID pBuffer
    )
{
    RBOOL isSuccess = FALSE;
    _rFile* pFile = (_rFile*)hFile;

    if( NULL != hFile &&
        NULL != pBuffer &&
        0 != size )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        RU32 written = 0;

        if( WriteFile( pFile->handle, pBuffer, size, (LPDWORD)&written, NULL ) &&
            written == size )
        {
            isSuccess = TRUE;
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( 1 == fwrite( pBuffer, size, 1, pFile->handle ) )
        {
            isSuccess = TRUE;
        }
#endif
    }

    return isSuccess;
}


rDirWatch
    rDirWatch_new
    (
        RPWCHAR dir,
        RU32 watchFlags,
        RBOOL includeSubDirs
    )
{
    _rDirWatch* watch = NULL;

    if( NULL != dir )
    {
        if( NULL != ( watch = rpal_memory_alloc( sizeof( _rDirWatch ) ) ) )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            watch->includeSubDirs = includeSubDirs;
            watch->flags = watchFlags;
            watch->curChange = NULL;
            watch->tmpTerminator = 0;
            watch->pTerminator = NULL;
            watch->isPending = FALSE;
            rpal_memory_zero( &(watch->oChange), sizeof( watch->oChange ) );

            if( NULL != ( watch->hDir = CreateFileW( dir, 
                                                     FILE_LIST_DIRECTORY | GENERIC_READ,
                                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                     NULL,
                                                     OPEN_EXISTING,
                                                     FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                                     NULL ) ) )
            {
                if( NULL == ( watch->hChange = CreateEventW( NULL, FALSE, FALSE, NULL ) ) )
                {
                    CloseHandle( watch->hDir );
                    rpal_memory_free( watch );
                    watch = NULL;
                }
            }
            else
            {
                rpal_memory_free( watch );
                watch = NULL;
            }
#endif
        }
    }

    return (rDirWatch)watch;
}

RVOID
    rDirWatch_free
    (
        rDirWatch watch
    )
{
    _rDirWatch* pWatch = (_rDirWatch*)watch;

    if( rpal_memory_isValid( pWatch ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        if( NULL != pWatch->hDir )
        {
            if( pWatch->isPending )
            {
                CancelIo( pWatch->hDir );

                while( !HasOverlappedIoCompleted( &(pWatch->oChange) ) )
                {
                    rpal_thread_sleep( 5 );
                }
            }
            CloseHandle( pWatch->hDir );
        }
        if( NULL != pWatch->hChange )
        {
            CloseHandle( pWatch->hChange );
        }
#endif
        rpal_memory_free( watch );
    }
}

RBOOL
    rDirWatch_next
    (
        rDirWatch watch,
        RU32 timeout,
        RPWCHAR* pFilePath,
        RU32* pAction
    )
{
    RBOOL gotChange = FALSE;

    _rDirWatch* volatile pWatch = (_rDirWatch*)watch;

    RU32 size = 0;

    if( rpal_memory_isValid( watch ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        // If we are at the end of our current buffer, refresh it with the OS
        if( NULL == pWatch->curChange ||
            ( (RPU8)pWatch->curChange < (RPU8)pWatch->changes ) || 
            ( (RPU8)pWatch->curChange >= (RPU8)pWatch->changes + sizeof( pWatch->changes ) ) )
        {
            if( !pWatch->isPending )
            {
                rpal_memory_zero( pWatch->changes, sizeof( pWatch->changes ) );
                pWatch->oChange.hEvent = pWatch->hChange;
            
                pWatch->curChange = NULL;
                pWatch->pTerminator = NULL;

                if( ReadDirectoryChangesW( pWatch->hDir, 
                                           &pWatch->changes, 
                                           sizeof( pWatch->changes ), 
                                           pWatch->includeSubDirs,
                                           pWatch->flags,
                                           (LPDWORD)&size,
                                           &(pWatch->oChange),
                                           NULL ) )
                {
                    pWatch->isPending = TRUE;
                }
            }

            if( pWatch->isPending )
            {
                if( WAIT_OBJECT_0 == WaitForSingleObject( pWatch->hChange, timeout ) )
                {
                    pWatch->isPending = FALSE;
                    pWatch->curChange = (FILE_NOTIFY_INFORMATION*)pWatch->changes;
                }
            }
        }

        // Return the next value
        if( NULL != pWatch->curChange )
        {
            gotChange = TRUE;

            // FileName is not NULL terminated (how awesome, MS)
            if( NULL != pWatch->pTerminator )
            {
                *pWatch->pTerminator = pWatch->tmpTerminator;
            }

            pWatch->pTerminator = (RPWCHAR)( (RPU8)pWatch->curChange->FileName + pWatch->curChange->FileNameLength );
            pWatch->tmpTerminator = *pWatch->pTerminator;
            *pWatch->pTerminator = 0;

            if( NULL != pFilePath )
            {
                *pFilePath = pWatch->curChange->FileName;
            }
            if( NULL != pAction )
            {
                *pAction = pWatch->curChange->Action;
            }
            if( 0 == pWatch->curChange->NextEntryOffset )
            {
                pWatch->curChange = NULL;
            }
            else
            {
                pWatch->curChange = (FILE_NOTIFY_INFORMATION*)( (RPU8)pWatch->curChange + pWatch->curChange->NextEntryOffset );
            }
        }
#else
        UNREFERENCED_PARAMETER( pWatch );
        UNREFERENCED_PARAMETER( size );
#endif
    }

    return gotChange;
}

