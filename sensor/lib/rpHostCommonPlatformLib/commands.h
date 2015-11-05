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

#ifndef _RP_HCP_COMMANDS_H
#define _RP_HCP_COMMANDS_H

#include <rpal/rpal.h>
#include <librpcm/librpcm.h>

#define RP_HCP_COMMAND_LOAD_MODULE          0x01
#define RP_HCP_COMMAND_UNLOAD_MODULE        0x02
#define RP_HCP_COMMAND_SET_NEXT_BEACON      0x03
#define RP_HCP_COMMAND_SET_HCP_ID           0x04
#define RP_HCP_COMMAND_SET_GLOBAL_TIME      0x05
#define RP_HCP_COMMAND_QUIT                 0x06


RBOOL
    processMessage
    (
        rSequence seq
    );

RBOOL
    stopAllModules
    (

    );

#endif

