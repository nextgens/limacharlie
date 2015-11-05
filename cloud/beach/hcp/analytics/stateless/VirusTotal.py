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
import virustotal
ObjectTypes = Actor.importLib( '../../ObjectsDb', 'ObjectTypes' )
StatelessActor = Actor.importLib( '../../Detects', 'StatelessActor' )
RingCache = Actor.importLib( '../../hcp_helpers', 'RingCache' )

class VirusTotal ( StatelessActor ):
    def init( self, parameters ):
        super( VirusTotal, self ).init( parameters )

        self.key = parameters.get( 'key', None )
        if self.key is None: raise Exception( 'missing API key' )

        # Maximum number of queries per minute
        self.qpm = parameters.get( 'qpm', 4 )

        self.vt = virustotal.VirusTotal( self.key, limit_per_min = self.qpm )

        # Minimum number of AVs saying it's a hit before we flag it
        self.threshold = parameters.get( 'min_av', 5 )

        # Cache size
        self.cache_size = parameters.get( 'cache_size', 200 )

        self.cache = RingCache( maxEntries = self.cache_size, isAutoAdd = False )

        # Todo: move from RingCache to using Cassandra as larger cache

    def process( self, msg ):
        routing, event, mtd = msg.data
        detects = []
        retries = 0
        for h in mtd[ 'obj' ].get( ObjectTypes.FILE_HASH, [] ):

            if h in self.cache:
                report = self.cache.get( h )
            else:
                while retries < 5:
                    info = self.vt.get( h )
                    if info is not None: break
                    retries += 1
                report = {}
                for av, r in info:
                    if r is not None:
                        report[ av[ 0 ] ] = r

                if self.threshold > len( report ):
                    report = None

                self.cache.add( h, report )

            if report is not None:
                detects.append( self.newDetec( objects = ( h, ), mtd = report ) )

        return detects