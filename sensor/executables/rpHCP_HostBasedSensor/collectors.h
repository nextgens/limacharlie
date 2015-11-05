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
#include <cryptoLib/cryptoLib.h>

typedef struct
{
    rEvent isTimeToStop;
    rThreadPool hThreadPool;
    rQueue outQueue;
    RU8 currentConfigHash[ CRYPTOLIB_HASH_SIZE ];
    RU32 maxQueueNum;
    RU32 maxQueueSize;
    RBOOL isProfilePresent;
} HbsState;

//=============================================================================
// Collector Naming Convention
//=============================================================================
#define DECLARE_COLLECTOR(num) RBOOL collector_ ##num## _init( HbsState* hbsState, \
                                                               rSequence config ); \
                               RBOOL collector_ ##num## _cleanup( HbsState* hbsState, \
                                                                  rSequence config );

#define ENABLED_COLLECTOR(num) { TRUE, collector_ ##num## _init, collector_ ##num## _cleanup, NULL }
#define DISABLED_COLLECTOR(num) { FALSE, collector_ ##num## _init, collector_ ##num## _cleanup, NULL }

#ifdef RPAL_PLATFORM_WINDOWS
    #define ENABLED_WINDOWS_COLLECTOR(num) ENABLED_COLLECTOR(num)
    #define ENABLED_LINUX_COLLECTOR(num) DISABLED_COLLECTOR(num)
    #define ENABLED_OSX_COLLECTOR(num) DISABLED_COLLECTOR(num)
    #define DISABLED_WINDOWS_COLLECTOR(num) DISABLED_COLLECTOR(num)
    #define DISABLED_LINUX_COLLECTOR(num) ENABLED_COLLECTOR(num)
    #define DISABLED_OSX_COLLECTOR(num) ENABLED_COLLECTOR(num)
#elif defined( RPAL_PLATFORM_LINUX )
    #define ENABLED_WINDOWS_COLLECTOR(num) DISABLED_COLLECTOR(num)
    #define ENABLED_LINUX_COLLECTOR(num) ENABLED_COLLECTOR(num)
    #define ENABLED_OSX_COLLECTOR(num) DISABLED_COLLECTOR(num)
    #define DISABLED_WINDOWS_COLLECTOR(num) ENABLED_COLLECTOR(num)
    #define DISABLED_LINUX_COLLECTOR(num) DISABLED_COLLECTOR(num)
    #define DISABLED_OSX_COLLECTOR(num) ENABLED_COLLECTOR(num)
#elif defined( RPAL_PLATFORM_MACOSX )
    #define ENABLED_WINDOWS_COLLECTOR(num) DISABLED_COLLECTOR(num)
    #define ENABLED_LINUX_COLLECTOR(num) DISABLED_COLLECTOR(num)
    #define ENABLED_OSX_COLLECTOR(num) ENABLED_COLLECTOR(num)
    #define DISABLED_WINDOWS_COLLECTOR(num) ENABLED_COLLECTOR(num)
    #define DISABLED_LINUX_COLLECTOR(num) ENABLED_COLLECTOR(num)
    #define DISABLED_OSX_COLLECTOR(num) DISABLED_COLLECTOR(num)
#endif

//=============================================================================
//  Declaration of all collectors for the HBS Core
//=============================================================================
DECLARE_COLLECTOR( 0 );
DECLARE_COLLECTOR( 1 );
DECLARE_COLLECTOR( 2 );
DECLARE_COLLECTOR( 3 );
DECLARE_COLLECTOR( 4 );
DECLARE_COLLECTOR( 5 );
DECLARE_COLLECTOR( 6 );
DECLARE_COLLECTOR( 7 );
DECLARE_COLLECTOR( 8 );
DECLARE_COLLECTOR( 9 );
DECLARE_COLLECTOR( 10 );
DECLARE_COLLECTOR( 11 );
DECLARE_COLLECTOR( 12 );
