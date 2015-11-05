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

#ifndef _RPAL_MODULE_H
#define _RPAL_MODULE_H

#include <rpal/rpal.h>

/*
 *
 * MODULE DEFINITIONS
 *
 */



// Required to initialize a master or slave rpal lib API.
RBOOL
RPAL_DONT_EXPORT
    rpal_initialize
    (
        rpal_PContext    context,
        RU32            logicalIdentifier
    );


#endif
