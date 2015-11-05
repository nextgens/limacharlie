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

#ifndef _RPAL_CHECKPOINT
#define _RPAL_CHECKPOINT

#include <rpal/rpal.h>

//=============================================================================
//  Fixed checkpoint positions. Numbers for these constants should increase
//  by +1 increments. The RPAL_CHECKPOINT_FIXED_NUMBER should remain the last
//  definition and, thus, end the increasing sequence.
//=============================================================================
#define RPAL_CHECKPOINT_ROLLING                            ((RU32)-1)
//#define RPAL_CHECKPOINT_FIXED_COLL0_ITER_START              0
//#define RPAL_CHECKPOINT_FIXED_COLL0_CREATE_NEW_SNAPSHOT     1
//#define RPAL_CHECKPOINT_FIXED_COLL0_SEEK_NEW_PROCESSES      2
//#define RPAL_CHECKPOINT_FIXED_COLL0_SEEK_DEAD_PROCESSES     3
//#define RPAL_CHECKPOINT_FIXED_COLL0_CALC_SLEEP_TIMEOUT      4
//#define RPAL_CHECKPOINT_FIXED_COLL0_ITER_END_SLEEP          5
//#define RPAL_CHECKPOINT_FIXED_NUMBER                        6
#define RPAL_CHECKPOINT_FIXED_HBS_SHUTDOWN                    0
#define RPAL_CHECKPOINT_FIXED_0_ITER_START                    1
#define RPAL_CHECKPOINT_FIXED_0_ITER_END                      2
#define RPAL_CHECKPOINT_FIXED_NUMBER                          3

//=============================================================================
//  PUBLIC STRUCTURES
//=============================================================================
typedef RPVOID rCheckpointHistory;

typedef struct
{
    RU64 timestamp;
    RU32 threadId;
    RU16 fileId;
    RU16 lineNum;
    RU16 unused;
    RU32 memUsed;

} rCheckpoint;

//=============================================================================
//  PUBLIC API
//=============================================================================
typedef RVOID (*rpal_checkpoint_cb)( RU32 checkpointArrayLen, rCheckpoint* checkpointArray );

rCheckpointHistory
    rpal_checkpoint_history_create
    (
        RU32 nCheckpoints,
        rpal_checkpoint_cb callback
    );

RVOID
    rpal_checkpoint_history_destroy
    (
        rCheckpointHistory history
    );

RBOOL
    rpal_checkpoint_history_get_snapshot
    (
        rCheckpointHistory history,
        RU32* pCheckpointArrayLen,
        rCheckpoint** pCheckpointArray
    );

// Defined mostly for internal values, see #define below
RBOOL
    rpal_checkpoint_checkinEx
    (
        rCheckpointHistory history,
        RU16 fileId,
        RU16 lineNumberOrId,
        RU32 index
    );

// Macro CHECKPOINT_FILE_ID must be defined in each file which checkpoints are
// thus used.
#define rpal_checkpoint_checkin(hist, tag)       rpal_checkpoint_checkinEx( (hist), CHECKPOINT_FILE_ID, __LINE__, RPAL_CHECKPOINT_ROLLING )

#define rpal_checkpoint_at(hist, position)       rpal_checkpoint_checkinEx( (hist), CHECKPOINT_FILE_ID, __LINE__, (position) )

#endif
