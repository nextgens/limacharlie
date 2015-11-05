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

#include <rpal/rpal_getopt.h>


RCHAR 
    rpal_getopt
    (
        RS32 argc, 
        RPCHAR* argv, 
        rpal_opt opts[],
        RPCHAR* pArgVal
    )
{
    static RS32 optind = 1;
    RCHAR nCmd = 0;
    RPCHAR pLongCmd = NULL;
    RCHAR retVal = -1;
    rpal_opt* pOpt = NULL;
    
    if( NULL != argv &&
        NULL != opts &&
        NULL != pArgVal )
    {
        if( argc > optind &&
            argv[ optind ] &&
            ( '-' == *argv[optind] ||
              ( '-' == argv[optind][0] &&
                '-' == argv[optind][1] ) ) )
        {
            if( '-' == argv[optind][0] &&
                '-' == argv[optind][1] )
            {
                pLongCmd = argv[ optind ] + 2;
            }
            else if( '-' == *argv[optind] )
            {
		        nCmd = *( ( argv[ optind ] + 1 ) );
            }

            if( 0 != nCmd ||
                NULL != pLongCmd )
            {
                pOpt = &opts[ 0 ];

                while( 0 != pOpt->shortSwitch )
                {
                    if( 0 != nCmd &&
                        0 != pOpt->shortSwitch &&
                        nCmd == pOpt->shortSwitch )
                    {
                        retVal = nCmd;
                    }
                    else if( NULL != pLongCmd &&
                             NULL != pOpt->longSwitch &&
                             0 == rpal_string_strcmpa( pLongCmd, pOpt->longSwitch ) )
                    {
                        retVal = pOpt->shortSwitch;
                    }

                    if( -1 != retVal )
                    {
                        if( !pOpt->hasArgument )
                        {
                            *pArgVal = NULL;
                            optind++;
                        }
                        else if( argc > optind + 1 )
                        {
                            *pArgVal = argv[ optind + 1 ];
                            optind += 2;
                        }
                        else
                        {
                            retVal = -1;
                            *pArgVal = NULL;
                            optind++;
                        }

                        break;
                    }

                    pOpt++;
                }
            }
	    }
    }

	return retVal;
}

