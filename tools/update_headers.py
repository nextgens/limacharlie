# Copyright 2015 refractionPOINT
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import sys
import json

rootDir = os.path.join( os.path.dirname( os.path.abspath( __file__ ) ), '..' )

inputTags = json.load( open( os.path.join( rootDir,
                                           'meta_headers',
                                           'rp_hcp_tags.json' ), 'r' ) )

#==============================================================================
# Output C Headers
#==============================================================================
cContent = \
[ '''
/* This file was automatically generated from JSON definitions. */

#ifndef _%s
#define _%s
''' % ( inputTags[ 'category' ],
        inputTags[ 'category' ] ) ]

tagValuesProcessed = {}
tagNamesProcessed = {}

for group in inputTags[ 'groups' ]:
    prefix = group[ 'namePrefix' ]
    print( "Processing group: %s" % prefix )

    for definition in group[ 'definitions' ]:
        tagName = definition[ 'name' ]
        tagValue = definition[ 'value' ]

        if tagValue in tagValuesProcessed:
            print( "Duplicate tag value detected: %s / %s AND %s" % ( tagName,
                                                                      tagValue,
                                                                      tagValuesProcessed[ tagValue ] ) )
            sys.exit( -1 )
        else:
            tagValuesProcessed[ tagValue ] = tagName

        if tagValue in tagNamesProcessed:
            print( "Duplicate tag name detected: %s / %s AND %s" % ( tagName,
                                                                     tagValue,
                                                                     tagNamesProcessed[ tagValue ] ) )
            sys.exit( -1 )
        else:
            tagNamesProcessed[ tagValue ] = tagName

        cContent.append( '#define %s%s %s' % ( prefix, tagName, tagValue ) )
cContent.append( '#endif' )

print( "Writing C definitions: %s" % os.path.join( rootDir, 'sensor', 'include', 'rpHostCommonPlatformLib', 'rTags.h' ) )
open( os.path.join( rootDir,
                    'sensor',
                    'include',
                    'rpHostCommonPlatformLib',
                    'rTags.h' ), 'w' ).write( '\n'.join( cContent ) )

#==============================================================================
# Output Cloud Headers
#==============================================================================
print( "Writing Cloud definitions: %s" % os.path.join( rootDir, 'cloud', 'beach', 'hcp', 'rp_hcp_tags.json' ) )
open( os.path.join( rootDir,
                    'cloud',
                    'beach',
                    'hcp',
                    'rp_hcp_tags.json' ), 'w' ).write( json.dumps( inputTags ) )
