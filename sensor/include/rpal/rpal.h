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

#ifndef _RPAL_H
#define _RPAL_H
/*
 *
 *	refractionPOINT Abstraction Library
 *
 * 
 */

// Common Macros
#define IS_FLAG_ENABLED(flags,toTest)	( 0 != ( (flags) & (toTest) ) )
#define ENABLE_FLAG(flags,toEnable)		( (flags) = (flags) | (toEnable) )
#define DISABLE_FLAG(flags,toDisable)	( (flags) = (flags) & ( (toDisable) ^ (toDisable) ) )
#define IS_WITHIN_BOUNDS(elem,elemSize,container,containerSize) (((RPU8)(elem) >= (RPU8)(container)) &&\
                                                                 ((RPU8)(elem) + (elemSize) <= (RPU8)(container) + (containerSize)))
#define ARRAY_N_ELEM(arr)               (sizeof(arr) / sizeof((arr)[0]))
#define IF_VALID_DO(elem,doFunc)        if( (elem) ){ doFunc( (elem) ); }
#define DO_IFF(op,bStatus)              if(bStatus){ bStatus = op; }
#define FREE_N_NULL(ptr,freeFunc)       freeFunc((ptr));(ptr)=NULL

#define FORCE_LINK_THIS(x) int force_link_##x = 0;
#define FORCE_LINK_THAT(x) { extern int force_link_##x; force_link_##x = 1; }

#define LOGICAL_XOR(a,b)    (((a) || (b)) && !((a) && (b)))

#define rpal_srand(seed)    srand(seed)
#define rpal_rand()         rand()

// Core features
#include <rpal_features.h>

// Core includes
#include <rpal_platform.h>
#include <rpal_datatypes.h>
#include <rpal_debug.h>
#include <rpal_va.h>
#include <rpal_getopt.h>
#include <rpal_error.h>

// Core routines
#include <rpal_context.h>
extern rpal_PContext g_rpal_context; // Central reference
#include <rpal_memory.h>
#include <rpal_time.h>
#include <rpal_module.h>
#include <rpal_endianness.h>
#include <rpal_synchronization.h>
#include <rpal_threads.h>

// Utility Objects
#include <rpal_string.h>
#include <rpal_blob.h>
#include <rpal_stringbuffer.h>
#include <rpal_array.h>
#include <rpal_file.h>
#include <rpal_btree.h>
#include <rpal_handleManager.h>
#include <rpal_handles.h>
#include <rpal_stack.h>
#include <rpal_queue.h>
#include <rpal_bloom.h>
#include <rpal_checkpoint.h>

// Enumerations
#include <rpal_handles.h>
#include <rpal_api.h>
#include <rpal_component.h>

#endif
