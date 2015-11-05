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
AgentId = Actor.importLib( '../hcp_helpers', 'AgentId' )

class AnalyticsStateless( Actor ):
    def init( self, parameters ):
        self.handleCache = {}
        self.modules = { 'notification.NEW_PROCESS' : [ 'analytics/stateless/suspexecloc/',
                                                        'analytics/stateless/batchselfdelete/',
                                                        'analytics/stateless/firewallclimods/',
                                                        'analytics/stateless/known_objects/' ],
                         'notification.TERMINATE_PROCESS' : [ 'analytics/stateless/suspexecloc/',
                                                              'analytics/stateless/batchselfdelete/' ],
                         'notification.CODE_IDENTITY' : [ 'analytics/stateless/virustotal/',
                                                          'analytics/stateless/known_objects/' ],
                         'notification.OS_SERVICES_REP' : [ 'analytics/stateless/virustotal/',
                                                            'analytics/stateless/known_objects/' ],
                         'notification.OS_DRIVERS_REP' : [ 'analytics/stateless/virustotal/',
                                                           'analytics/stateless/known_objects/' ],
                         'notification.OS_AUTORUNS_REP' : [ 'analytics/stateless/virustotal/',
                                                            'analytics/stateless/known_objects/' ],
                         'notification.DNS_REQUEST' : [ 'analytics/stateless/known_objects/' ] }

        self.handle( 'analyze', self.analyze )

    def _sendForProcessing( self, cat, msg ):
        if cat not in self.handleCache:
            self.handleCache[ cat ] = self.getActorHandle( cat, timeout = 30, nRetries = 3 )
        self.handleCache[ cat ].shoot( 'process', msg )

    def deinit( self ):
        pass

    def analyze( self, msg ):
        routing, event, mtd = msg.data

        toRun = self.modules.get( routing[ 'event_type' ], [] )
        for m in toRun:
            self._sendForProcessing( m, msg.data )

        return ( True, )