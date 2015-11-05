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

#ifndef _RPAL_ERROR_H
#define _RPAL_ERROR_H

#include <rpal.h>


typedef RU32 RERROR;



// error numbers follows the numbering system and error codes from WinError.h 

#define RPAL_ERROR_SUCCESS                  0
#define RPAL_ERROR_INVALID_FUNCTION         1
#define RPAL_ERROR_FILE_NOT_FOUND           2
#define RPAL_ERROR_PATH_NOT_FOUND           3
#define RPAL_ERROR_ACCESS_DENIED            5
#define RPAL_ERROR_INVALID_HANDLE           6
#define RPAL_ERROR_NOT_ENOUGH_MEMORY        8
#define RPAL_ERROR_INVALID_DRIVE            15
#define RPAL_ERROR_CURRENT_DIRECTORY        16
#define RPAL_ERROR_WRITE_PROTECT            19
#define RPAL_ERROR_CRC                      23
#define RPAL_ERROR_SEEK                     25
#define RPAL_ERROR_WRITE_FAULT              29
#define RPAL_ERROR_READ_FAULT               30
#define RPAL_ERROR_SHARING_VIOLATION        32
#define RPAL_ERROR_LOCK_VIOLATION           33
#define RPAL_ERROR_HANDLE_EOF               38
#define RPAL_ERROR_HANDLE_DISK_FULL         39
#define RPAL_ERROR_NOT_SUPPORTED            50
#define RPAL_ERROR_BAD_NETPATH              53
#define RPAL_ERROR_NETWORK_BUSY             54
#define RPAL_ERROR_NETWORK_ACCESS_DENIED    65
#define RPAL_ERROR_BAD_NET_NAME             67
#define RPAL_ERROR_FILE_EXISTS              80
#define RPAL_ERROR_INVALID_PASSWORD         86
#define RPAL_ERROR_INVALID_PARAMETER        87
#define RPAL_ERROR_BROKEN_PIPE              109
#define RPAL_ERROR_OPEN_FAILED              110
#define RPAL_ERROR_BUFFER_OVERFLOW          111
#define RPAL_ERROR_DISK_FULL                112
#define RPAL_ERROR_INVALID_NAME             123
#define RPAL_ERROR_NEGATIVE_SEEK            131
#define RPAL_ERROR_DIR_NOT_EMPTY            145
#define RPAL_ERROR_BUSY                     170
#define RPAL_ERROR_BAD_EXE_FORMAT           193
#define RPAL_ERROR_FILENAME_EXCED_RANGE     206
#define RPAL_ERROR_FILE_TOO_LARGE           223
#define RPAL_ERROR_DIRECTORY                267
#define RPAL_ERROR_INVALID_ADDRESS          487



RERROR rpal_error_rErrorFromOsError( RU32 osError );

RERROR rpal_error_getLast();
#endif // _RPAL_ERROR_H

