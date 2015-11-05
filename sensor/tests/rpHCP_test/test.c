#include <math.h>

#include <rpal/rpal.h>
#include <rpHostCommonPlatformIFaceLib/rpHostCommonPlatformIFaceLib.h>
#include <rpHostCommonPlatformLib/rTags.h>

#define RPAL_FILE_ID      90


#pragma warning( disable : 4127 )

//=============================================================================
//  RP HCP Module Requirements
//=============================================================================
RpHcp_ModuleId g_current_Module_id = RPAL_COMPONENT_TEST;

#define RP_HCP_TEST_BEACON_DELTA      (10)

#define RP_HCP_TEST_NB_BEACON_THREADS  (8)


typedef struct
{
    int index;
    rThread thread;
    rEvent eventStop;
} beaconTestThreadContext;


RU32
RPAL_THREAD_FUNC
    thread_beacon_test
    (
        RPVOID ctx
    )
{
    beaconTestThreadContext* context = (beaconTestThreadContext*)ctx;
    RU32 timeToBeacon = 0;
    rList data = NULL;
    rList resp = NULL;

    while( TRUE )
    {
        // Using exponential random distribution of inter-beacon times.
        // Average is 1000 ms.
        timeToBeacon = -1000 * (RU32)log(1.0 - rpal_rand());
        if ( rEvent_wait( context->eventStop, timeToBeacon ) )
        {
            break;
        }

        if( NULL != ( data = rList_new( RP_TAGS_MESSAGE, RPCM_SEQUENCE ) ) )
        {
            rSequence seq = NULL;
            if( NULL != ( seq = rSequence_new() ) )
            {
                RU32 nb_ints = (RU32)( rpal_rand() * 256.0 );
                RU32 i = 0;

                for( i = 0; i < nb_ints; ++i )
                {
                    rSequence_addRU32(
                            seq,
                            RP_TAGS_HCP_REVISION_NUMBER,
                            (RU32)( rand() * (double)4000000000ul )
                            );
                }
                rList_addSEQUENCE( data, seq );
            }
            else
            {
                rpal_debug_error( "Beacon test thread %d unable to create inner sequence.", context->index );
            }
        }
        else
        {
            rpal_debug_error( "Beacon test thread %d unable to create data list to send.", context->index );
        }

        rpal_debug_info( "At %llu: beacon test thread %d beaconing.", rpal_time_getGlobal(), context->index );
        if( !rpHcpI_beaconHome( data, &resp ) )
        {
            rpal_debug_warning( "Beacon failed for test thread %d.", context->index );
        }

        rList_free( data );
        data = NULL;
        rList_free( resp );
        resp = NULL;
    }

    return 0;
}


RU32
RPAL_THREAD_FUNC
    RpHcpI_mainThread
    (
        rEvent eventStop
    )
{
    rEvent eventStopBeacon = NULL;
    beaconTestThreadContext arrayThreadBeacon[ RP_HCP_TEST_NB_BEACON_THREADS ] = {0};
    RBOOL isSuccess = TRUE;
    RU32 ret = 0;
    int i;

    FORCE_LINK_THAT(HCP_IFACE);
    rpal_debug_info( "Starting up beacon test module." );

    if ( NULL != ( eventStopBeacon = rEvent_create( TRUE ) ) )
    {
        for( i = 0; i < RP_HCP_TEST_NB_BEACON_THREADS; ++i )
        {
            arrayThreadBeacon[ i ].index = i;
            arrayThreadBeacon[ i ].eventStop = eventStopBeacon;
            arrayThreadBeacon[ i ].thread = rpal_thread_new( thread_beacon_test, arrayThreadBeacon + i );
            if ( 0 == arrayThreadBeacon[ i ].thread )
            {
                isSuccess = FALSE;
                break;
            }
        }

        if( isSuccess )
        {
            rpal_debug_info(
                    "Set up all %d beacon test threads. Now it's the waiting game.",
                    RP_HCP_TEST_NB_BEACON_THREADS
                    );
            rEvent_wait( eventStop, RINFINITE );
        }
        else
        {
            rpal_debug_error( "Unable to start all beacon threads. Abort." );
            ret = 1;
        }
    }

    rEvent_set( eventStopBeacon );
    for( i = 0; i < RP_HCP_TEST_NB_BEACON_THREADS; ++i )
    {
        if( 0 != arrayThreadBeacon[ i ].thread )
        {
            rpal_thread_wait( arrayThreadBeacon[ i ].thread, RINFINITE );
            rpal_thread_free( arrayThreadBeacon[ i ].thread );
        }
    }
    rEvent_free( eventStopBeacon );
    eventStopBeacon = NULL;

    return ret;
}


// EOF
