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

#include <rpal/rpal_api.h>


RBOOL
    rpal_api_register
    (
        RU32 componentId,
        rpal_apiHeader* api,
        rpal_handleManager_cleanup_f cleanup
    )
{
    RBOOL isSuccess = FALSE;
    rHandle hTmp = RPAL_HANDLE_INIT;

    if( NULL != api )
    {
        hTmp = rpal_handleManager_create( RPAL_HANDLES_MAJOR_APIS, componentId, api, cleanup );

        if( RPAL_HANDLE_INVALID != hTmp.h )
        {
            api->hApi = hTmp;

            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    rpal_api_acquire
    (
        RU32 componentId,
        rpal_apiHeader** pApi
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != pApi )
    {
        if( rpal_handleManager_openEx( RPAL_HANDLES_MAJOR_APIS, componentId, (RPVOID*)pApi ) )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    rpal_api_release
    (
        rpal_apiHeader* api
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != api )
    {
        if( rpal_handleManager_close( api->hApi, NULL ) )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess = FALSE;
}


