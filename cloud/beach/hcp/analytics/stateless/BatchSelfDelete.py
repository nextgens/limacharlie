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

class BatchSelfDelete ( StatelessActor ):
    def init( self, parameters ):
        super( BatchSelfDelete, self ).init( parameters )

        self.self_del = re.compile( r'.*\& +(?:(?:del)|(?:rmdir)) .*', re.IGNORECASE )

    def process( self, msg ):
        routing, event, mtd = msg.data
        detects = []
        for o in mtd[ 'obj' ].get( ObjectTypes.CMD_LINE, [] ):
            if self.self_del.search( o ):
                detects.append( self.newDetect( objects = ( o, ObjectTypes.CMD_LINE ) ) )

        return detects