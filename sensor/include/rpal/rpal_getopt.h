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

#ifndef _RPAL_GETOPT_H
#define _RPAL_GETOPT_H

#include <rpal.h>

/*
POSIX getopt for Windows

AT&T Public License

Code given out at the 1985 UNIFORUM conference in Dallas.  
*/

typedef struct
{
    RCHAR shortSwitch;
    RPCHAR longSwitch;
    RBOOL hasArgument;

} rpal_opt;

RCHAR 
    rpal_getopt
    (
        RS32 argc, 
        RPCHAR* argv, 
        rpal_opt opts[],
        RPCHAR* pArgVal
    );


#endif
