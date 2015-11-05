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

#include <rpal/rpal_error.h>

RERROR rpal_error_rErrorFromOsError( RU32 osError )
{
    RERROR err = 0xffffffff;
#ifdef RPAL_PLATFORM_WINDOWS
    UNREFERENCED_PARAMETER( err );
    return osError;             // simple case because rpal errors are aligned with definitions in WinErrors.h
#elif defined( RPAL_PLATFORM_LINUX ) || ( defined( RPAL_PLATFORM_MACOSX ) )
    switch ( osError ) 
    {
        case 0 : err = RPAL_ERROR_SUCCESS; break;
        case EACCES : err = RPAL_ERROR_ACCESS_DENIED; break;
        case EEXIST : err = RPAL_ERROR_FILE_EXISTS; break;
        case ENOTEMPTY : err = RPAL_ERROR_DIR_NOT_EMPTY; break;
        case EBUSY : err = RPAL_ERROR_BUSY; break;

        // todo: add more unix error to rpal error conversions...
    }

    return err;

#endif
}

RERROR rpal_error_getLast()
{
#ifdef RPAL_PLATFORM_WINDOWS
    return GetLastError();
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    return rpal_error_rErrorFromOsError( errno );
#endif
}

