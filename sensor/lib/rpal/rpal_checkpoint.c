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

#include <rpal/rpal_checkpoint.h>
#include <rpal/rpal_time.h>
#include <rpal/rpal_synchronization.h>

#define RPAL_FILE_ID   5

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
#include <pthread.h>
#endif

typedef struct
{
    RU32 nCheckpoints;     // Number of ringbuffer checkpoints; fixed checkpoints are stored at
    RU32 nNextCheckpoint;  // the end of the checkpoints[] array.
    rMutex mutex;
    rpal_checkpoint_cb cb;
    rpal_hires_timestamp_metadata ts_meta;
    rCheckpoint checkpoints[];
} _rCheckpointHistory, *_rPCheckpointHistory;


#define checkpoint_array_len( n )   ( ( n ) + RPAL_CHECKPOINT_FIXED_NUMBER )


rCheckpointHistory
    rpal_checkpoint_history_create
    (
        RU32 nCheckpoints,
        rpal_checkpoint_cb callback
    )
{
    _rPCheckpointHistory hist = NULL;

    if( 0 != nCheckpoints )
    {
        if( NULL != ( hist = rpal_memory_alloc( sizeof( _rCheckpointHistory ) + ( checkpoint_array_len( nCheckpoints ) * sizeof( rCheckpoint ) ) ) ) )
        {
            if( NULL != ( hist->mutex = rMutex_create() ) )
            {
                hist->nCheckpoints = nCheckpoints;
                hist->nNextCheckpoint = 0;
                hist->cb = callback;
                rpal_time_hires_timestamp_metadata_init( &hist->ts_meta );
                rpal_memory_zero( hist->checkpoints, sizeof( rCheckpoint ) * checkpoint_array_len( nCheckpoints ) );
            }
            else
            {
                rpal_memory_free( hist );
                hist = NULL;
            }
        }
    }

    return hist;
}

RVOID
    rpal_checkpoint_history_destroy
    (
        rCheckpointHistory history
    )
{
    _rPCheckpointHistory hist = (_rPCheckpointHistory)history;

    if( NULL != history &&
        rMutex_lock( hist->mutex ) )
    {
        rMutex_free( hist->mutex );
        rpal_memory_free( hist );
    }
}


RBOOL
    _rpal_checkpoint_history_get_snapshot
    (
        rCheckpointHistory history,
        RU32* pCheckpointArrayLen,
        rCheckpoint** pCheckpointArray
    )
{
    RBOOL isSuccess = FALSE;
    _rPCheckpointHistory hist = (_rPCheckpointHistory)history;
    rCheckpoint* snap = NULL;
    RU32 start = 0;

    if( NULL != history &&
        NULL != pCheckpointArray &&
        NULL != pCheckpointArrayLen )
    {
        if( NULL != ( snap = rpal_memory_alloc( checkpoint_array_len( hist->nCheckpoints ) * sizeof( rCheckpoint ) ) ) )
        {
            start = hist->nNextCheckpoint;

            if( 0 == start )
            {
                start = hist->nCheckpoints - 1;
            }

            rpal_memory_memcpy( snap, hist->checkpoints + start, ( hist->nCheckpoints - start ) * sizeof( rCheckpoint ) );
            if( 0 != start )
            {
                rpal_memory_memcpy( snap + ( hist->nCheckpoints - start ), hist->checkpoints, start * sizeof( rCheckpoint ) );
            }

#if 0 < RPAL_CHECKPOINT_FIXED_NUMBER
            rpal_memory_memcpy( snap + hist->nCheckpoints, hist->checkpoints + hist->nCheckpoints, RPAL_CHECKPOINT_FIXED_NUMBER * sizeof( rCheckpoint ) );
#endif

            *pCheckpointArray = snap;
            *pCheckpointArrayLen = checkpoint_array_len( hist->nCheckpoints );

            isSuccess = TRUE;
        }
    }

    return isSuccess;
}


RBOOL
    rpal_checkpoint_history_get_snapshot
    (
        rCheckpointHistory history,
        RU32* pCheckpointArrayLen,
        rCheckpoint** pCheckpointArray
    )
{
    RBOOL isSuccess = FALSE;
    _rPCheckpointHistory hist = (_rPCheckpointHistory)history;

    if( NULL != history &&
        NULL != pCheckpointArray &&
        NULL != pCheckpointArrayLen &&
        rMutex_lock( hist->mutex ) )
    {
        isSuccess = _rpal_checkpoint_history_get_snapshot( history, pCheckpointArrayLen, pCheckpointArray );

        rMutex_unlock( hist->mutex );
    }

    return isSuccess;
}


RBOOL
    rpal_checkpoint_checkinEx
    (
        rCheckpointHistory history,
        RU16 fileId,
        RU16 lineNumberOrId,
        RU32 index
    )
{
    RBOOL isSuccess = FALSE;
    _rPCheckpointHistory hist = (_rPCheckpointHistory)history;
    rCheckpoint tmp = {0};

    RU32 tmpHistLen = 0;
    rCheckpoint* tmpHist = NULL;

    if( NULL != history )
    {
        tmp.fileId = fileId;
        tmp.lineNum = lineNumberOrId;
        tmp.memUsed = rpal_memory_totalUsed();
#ifdef RPAL_PLATFORM_WINDOWS
        tmp.threadId = GetCurrentThreadId();
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        tmp.threadId = (RU32)pthread_self();
#endif
        tmp.timestamp = rpal_time_get_hires_timestamp( &hist->ts_meta );

        if( rMutex_lock( hist->mutex ) )
        {
            if ( RPAL_CHECKPOINT_ROLLING == index )
            {
                hist->checkpoints[ hist->nNextCheckpoint ] = tmp;
                hist->nNextCheckpoint++;

                if( hist->nNextCheckpoint >= hist->nCheckpoints )
                {
                    hist->nNextCheckpoint = 0;
                }
            }
            else
            {
                hist->checkpoints[ hist->nCheckpoints + index ] = tmp;
            }

            // We provide the callback with a snapshot every time. It's the CB's job to free the history.
            if( NULL != hist->cb )
            {
                if( _rpal_checkpoint_history_get_snapshot( history, &tmpHistLen, &tmpHist ) )
                {
                    hist->cb( tmpHistLen, tmpHist );
                }
            }

            rMutex_unlock( hist->mutex );

            rpal_debug_info( "checkpoint --- line %d --- %d Kbytes", lineNumberOrId, tmp.memUsed / 1024 );
        }
    }

    return isSuccess;
}
