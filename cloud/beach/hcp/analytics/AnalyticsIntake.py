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

from sets import Set
from beach.actor import Actor
_xm_ = Actor.importLib( '../hcp_helpers', '_xm_' )
_x_ = Actor.importLib( '../hcp_helpers', '_x_' )
exeFromPath = Actor.importLib( '../hcp_helpers', 'exeFromPath' )
ObjectTypes = Actor.importLib( '../ObjectsDb', 'ObjectTypes' )
ObjectNormalForm = Actor.importLib( '../ObjectsDb', 'ObjectNormalForm' )
AgentId = Actor.importLib( '../hcp_helpers', 'AgentId' )

class AnalyticsIntake( Actor ):
    def init( self, parameters ):
        self.handle( 'analyze', self.analyze )
        self.analytics_stateless = self.getActorHandle( 'analytics/stateless/intake', timeout = 30, nRetries = 3 )
        self.analytics_stateful = self.getActorHandle( 'analytics/stateful/intake', timeout = 30, nRetries = 3 )
        self.analytics_modeling = self.getActorHandle( 'analytics/modeling/intake', timeout = 120, nRetries = 3 )

    def deinit( self ):
        pass

    def _addObj( self, mtd, o, oType ):
        mtd[ 'obj' ].setdefault( oType, Set() ).add( o )

    def _addRel( self, mtd, parent, parentType, child, childType ):
        mtd[ 'rel' ].setdefault( ( parentType, childType ), Set() ).add( ( parent, child ) )

    def _extractProcess( self, agent, mtd, procRoot ):
        exePath = procRoot.get( 'base.FILE_PATH', None )
        exe = None
        if exePath is not None:
            exe = exeFromPath( exePath, agent = agent )

            self._addObj( mtd, exePath, ObjectTypes.FILE_PATH )
            self._addObj( mtd, exe, ObjectTypes.PROCESS_NAME )
            self._addRel( mtd, exe, ObjectTypes.PROCESS_NAME, exePath, ObjectTypes.FILE_PATH )
            cmdLine = procRoot.get( 'base.COMMAND_LINE', None )
            if cmdLine is not None:
                self._addObj( mtd, cmdLine, ObjectTypes.CMD_LINE )
                self._addRel( mtd, exe, ObjectTypes.PROCESS_NAME, cmdLine, ObjectTypes.CMD_LINE )
        return exe

    def _extractModule( self, agent, mtd, modRoot ):
        modPath = modRoot.get( 'base.FILE_PATH', None )
        modName = modRoot.get( 'base.MODULE_NAME', None )
        mod = modName if modName is not None else exeFromPath( modPath, agent = agent )
        memSize = modRoot.get( 'base.MEMORY_SIZE', None )

        if modName is not None:
            self._addObj( mtd, modName, ObjectTypes.MODULE_NAME )

        if modPath is not None:
            self._addObj( mtd, modPath, ObjectTypes.FILE_PATH )

        if memSize is not None:
            self._addObj( mtd, memSize, ObjectTypes.MODULE_SIZE )
            if mod is not None:
                self._addRel( mtd, mod, ObjectTypes.MODULE_NAME, memSize, ObjectTypes.MODULE_SIZE )

        return mod

    def _extractService( self, agent, mtd, svcRoot ):
        svcname = svcRoot.get( 'base.SVC_NAME', None )
        displayname = svcRoot.get( 'base.SVC_DISPLAY_NAME', None )
        exe = svcRoot.get( 'base.EXECUTABLE', None )
        dll = svcRoot.get( 'base.DLL', None )
        h = svcRoot.get( 'base.HASH', None )

        mainMod = exe
        if dll is not None:
            mainMod = dll

        if svcname is not None:
            self._addObj( mtd, svcname, ObjectTypes.SERVICE_NAME )
        if displayname is not None:
            self._addObj( mtd, displayname, ObjectTypes.SERVICE_NAME )

        if svcname is not None and displayname is not None:
            self._addRel( mtd, svcname, ObjectTypes.SERVICE_NAME, displayname, ObjectTypes.SERVICE_NAME )

        self._addObj( mtd, mainMod, ObjectTypes.FILE_PATH )
        self._addRel( mtd, svcname, ObjectTypes.SERVICE_NAME, dll, ObjectTypes.FILE_PATH )
        if h is not None:
            self._addObj( mtd, h, ObjectTypes.FILE_HASH )
            self._addRel( mtd, svcname, ObjectTypes.SERVICE_NAME, h, ObjectTypes.FILE_HASH )

    def _extractAutoruns( self, agent, mtd, aRoot ):
        reg = aRoot.get( 'base.REGISTRY_KEY', None )
        path = aRoot.get( 'base.FILE_PATH', None )
        h = aRoot.get( 'base.HASH', None )
        if reg is not None:
            autorun = reg
        elif path is not None:
            autorun = path

        if autorun is not None:
            self._addObj( mtd, autorun, ObjectTypes.AUTORUNS )
            if h is not None:
                self._addObj( mtd, h, ObjectTypes.FILE_HASH )
                self._addRel( mtd, autorun, ObjectTypes.AUTORUNS, h, ObjectTypes.FILE_HASH )

    def _extractCodeIdentity( self, agent, mtd, cRoot ):
        filePath = cRoot.get( 'base.FILE_PATH', None )
        fileName = exeFromPath( filePath, agent = agent )
        h = cRoot.get( 'base.HASH', None )

        if filePath is not None:
            self._addObj( mtd, filePath, ObjectTypes.FILE_PATH )
            self._addObj( mtd, fileName, ObjectTypes.MODULE_NAME )

        if h is not None:
            self._addObj( mtd, h, ObjectTypes.FILE_HASH )
            if fileName is not None:
                self._addRel( mtd, fileName, ObjectTypes.MODULE_NAME, h, ObjectTypes.FILE_HASH )

    def _extractObjects( self, message ):
        routing, event = message
        mtd = { 'obj' : {}, 'rel' : {}, 'k' : [] }
        agent = AgentId( routing[ 'agentid' ] )
        eventType = routing.get( 'event_type', None )
        eventRoot = event.values()[ 0 ]

        if eventType in ( 'notification.NEW_PROCESS',
                          'notification.TERMINATE_PROCESS' ):
            curExe = self._extractProcess( agent, mtd, eventRoot )
            parent = eventRoot.get( 'base.PARENT', None )
            if parent is not None:
                parentExe = self._extractProcess( agent, mtd, parent )
                if parentExe is not None:
                    self._addRel( mtd, parentExe, ObjectTypes.PROCESS_NAME, curExe, ObjectTypes.PROCESS_NAME )
        elif eventType in ( 'notification.DNS_REQUEST', ):
            self._addObj( mtd, eventRoot[ 'base.DOMAIN_NAME' ], ObjectTypes.DOMAIN_NAME )
        elif eventType in ( 'notification.OS_PROCESSES_REP', ):
            for p in eventRoot[ 'base.PROCESSES' ]:
                exe = self._extractProcess( agent, mtd, p )
                for m in p[ 'base.MODULES' ]:
                    mod = self._extractModule( agent, mtd, m )
                    self._addRel( mtd, exe, ObjectTypes.PROCESS_NAME,
                                       mod, ObjectTypes.MODULE_NAME )
        elif eventType in ( 'notification.NETWORK_SUMMARY', ):
            exe = self._extractProcess( agent, mtd, eventRoot )
            for c in eventRoot.get( 'base.NETWORK_ACTIVITY', [] ):
                port = c.get( 'base.DESTINATION', {} ).get( 'base.PORT', None )
                if port is not None:
                    self._addObj( mtd, port, ObjectTypes.PORT )
                    if exe is not None:
                        self._addRel( mtd, exe, ObjectTypes.PROCESS_NAME, port, ObjectTypes.PORT )
        elif eventType in ( 'notification.OS_SERVICES_REP',
                            'notification.OS_DRIVERS_REP' ):
            for s in eventRoot[ 'base.SVCS' ]:
                self._extractService( agent, mtd, s )
        elif eventType in ( 'notification.OS_AUTORUNS_REP', ):
            for a in eventRoot[ 'base.AUTORUNS' ]:
                self._extractAutoruns( agent, mtd, a )
        elif eventType in ( 'notification.CODE_IDENTITY', ):
            self._extractCodeIdentity( agent, mtd, eventRoot )

        # Ensure we cleanup the mtd
        self._convertToNormalForm( mtd, not agent.isWindows() )

        return ( routing, event, mtd )

    def _convertToNormalForm( self, mtd, isCaseSensitive ):
        k = []
        for oType in mtd[ 'obj' ].keys():
            mtd[ 'obj' ][ oType ] = [ ObjectNormalForm( x, oType, isCaseSensitive = isCaseSensitive ) for x in mtd[ 'obj' ][ oType ] ]
        for ( parentType, childType ) in mtd[ 'rel' ].keys():
            mtd[ 'rel' ][ ( parentType, childType ) ] = [ ( ObjectNormalForm( x[ 0 ], parentType, isCaseSensitive = isCaseSensitive ), ObjectNormalForm( x[ 1 ], childType, isCaseSensitive = isCaseSensitive ) ) for x in mtd[ 'rel' ][ ( parentType, childType ) ] ]

    def analyze( self, msg ):

        for event in msg.data:
            # Enhance the data
            event = self._extractObjects( event )

            # Send the events to actual analysis
            self.analytics_modeling.shoot( 'analyze', event )
            self.analytics_stateless.shoot( 'analyze', event )
            self.analytics_stateful.shoot( 'analyze', event )

        return ( True, )


