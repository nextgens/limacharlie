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

from beach.actor import Actor
import re
ObjectTypes = Actor.importLib( '../../ObjectsDb', 'ObjectTypes' )
StatelessActor = Actor.importLib( '../../Detects', 'StatelessActor' )

class SuspExecLoc ( StatelessActor ):
    def init( self, parameters ):
        super( SuspExecLoc, self ).init( parameters )
        self.slocs = { 'tasks' : re.compile( r'.*windows\\(?:(?:system32)||(?:syswow64))\\tasks\\.*', re.IGNORECASE ),
                       'recycler' : re.compile( r'.*recycle.*', re.IGNORECASE ),
                       'fonts' : re.compile( r'.*\\windows\\fonts\\.*', re.IGNORECASE ),
                       'help' : re.compile( r'.*\\windows\\help\\.*', re.IGNORECASE ),
                       'wbem' : re.compile( r'.*\\windows\\wbem\\.*', re.IGNORECASE ),
                       'addins' : re.compile( r'.*\\windows\\addins\\.*', re.IGNORECASE ),
                       'debug' : re.compile( r'.*\\windows\\debug\\.*', re.IGNORECASE ),
                       'virt_device' : re.compile( r'\\\\\\.\\.*', re.IGNORECASE ) }

    def process( self, msg ):
        routing, event, mtd = msg.data
        detects = []
        for o in mtd[ 'obj' ].get( ObjectTypes.FILE_PATH, [] ):
            for k, v in self.slocs.iteritems():
                if v.search( o ):
                    detects.append( self.newDetec( objects = ( o, ObjectTypes.FILE_PATH ) ) )

        return detects