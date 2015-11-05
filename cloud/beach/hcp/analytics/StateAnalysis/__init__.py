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

import time

from ...hcp_helpers import _x_
from ...hcp_helpers import _xm_


class _BoundedList( list ):

    def __init__( self, minSize, minRealTime = None, minWaves = None ):
        self.minSize = minSize
        self.minRealTime = minRealTime
        self.lastPurge = 0
        self.lastUpdate = 0
        self.waveCounter = 0
        self.minWaves = minWaves
        self.overflowAtPurge = 0

    def balance( self ):
        curTime = time.time()
        curSize = len( self )
        if self.minSize < curSize:
            if( ( None == self.minRealTime and None == self.minWaves ) or
                ( self.minRealTime is not None and ( curTime - self.lastPurge ) >= self.minRealTime ) or
                ( self.minWaves is not None and ( self.waveCounter > self.minWaves ) ) ):
                print( "Balance starting" )
                self.lastPurge = curTime
                nRemoved = 0
                newOverflow = curSize - self.minSize
                while ( curSize > self.minSize ) and ( nRemoved < self.overflowAtPurge ):
                    self.pop( 0 )
                    curSize -= 1
                    nRemoved +=1
                self.overflowAtPurge = newOverflow
                self.waveCounter = 0

    def add( self, elem ):
        self.append( elem )
        curTime = time.time()
        if 10 <= curTime - self.lastUpdate:
            self.waveCounter += 1
        self.lastUpdate = curTime
        self.balance()

class SAMWidget( object ):

    def __init__( self, parameters = {}, isSegregateByAid = True ):
        self.isByAid = isSegregateByAid
        self.parameters = parameters
        self.isDebug = False
        self.keys = None

        if 'debug' in parameters and parameters[ 'debug' ] is True:
            self.isDebug = True

    def _execute( self, event ):
        raise Exception( 'needs implementation' )

    def reportKey( self, keyType, keyPath ):
        if self.keys is None:
            self.keys = []
        self.keys.append( [ keyType, keyPath ] )
        return self

class SAMStateWidget( SAMWidget ):

    def __init__( self, *a, **k ):
        SAMWidget.__init__( self, *a, **k )

        self.children = []
        self.states = {}
        self.lastCleanup = 0
        self.isProcessShutdown = hasattr( self, 'onShutdown' )
        self.currentlyProcessingKey = None

    def addContributor( self, widget, stateSize, minStateLife = None ):
        self.children.append( [ widget, stateSize, minStateLife ] )
        return self

    def genState( self ):
        return [ x + [ _BoundedList( x[ 1 ], x[ 2 ], 2 ) ] for x in self.children ]

    def _execute( self, event ):
        outputState = []

        # Do a complete cleanup of states every 60 seconds
        # This is useful if an agent spiked and then stopped talking for a while
        if time.time() > ( self.lastCleanup + ( 60 * 1) ):
            for x in self.states.values():
                for y in x:
                    y[ 3 ].balance()
            self.lastCleanup = time.time()

        # Default single-state
        k = 0

        # If segregating by Aid
        if self.isByAid:
            k = _x_( event, 'routing/agentid' )

        # New states get initialized
        if k not in self.states:
            self.states[ k ] = self.genState()

        self.currentlyProcessingKey = k

        # We are doing depth-first evaluation
        isNewStateGenerated = False
        for c in self.states[ k ]:
            newStates = c[ 0 ]._execute( event )
            if 0 != len( newStates ):
                for tmpState in newStates:
                    c[ 3 ].add( tmpState )
                isNewStateGenerated = True

        # If the current widget implements the onShutdown function
        # we will call it before normal execution. This is usually
        # used when a widget must reset large parts of its state on
        # reboots, which is where agent shutdown usually occcurs.
        # We also use STARTING_UP to cover cases where sensor crashes.
        if self.isProcessShutdown and ( _x_( event, 'notification.SHUTTING_DOWN' ) is not None or _x_( event, 'notification.STARTING_UP' ) ):
            self.onShutdown( [ x[ 3 ] for x in self.states[ k ] ] )

        # Only evaluate the current widget if a child
        # generated a new piece of data
        if isNewStateGenerated:
            outputState = self.execute( [ x[ 3 ] for x in self.states[ k ] ] )

        if self.keys is not None:
            # We have report keys, so we must be a reporting root
            reportKeys = {}
            for k in self.keys:
                newKs = _xm_( outputState, k[ 1 ] )
                if newKs is not None and 0 != len( newKs ):
                    reportKeys.setdefault( k[ 0 ], [] ).extend( newKs )
            return [ outputState, reportKeys ]

        if self.isDebug:
            print( "Input size: %d, output size: %d" % ( len( event ), len( outputState ) ) )

        return outputState

    # This function is available to the widget to do
    # more complex updates to the current states
    def setStates( self, states ):
        k = self.currentlyProcessingKey
        for i in xrange( 0, len( states ) ):
            del self.states[ k ][ i ][ 3 ][ : ]
            for x in states[ i ]:
                self.states[ k ][ i ][ 3 ].append( x )

    def execute( self, states ):
        raise Exception( 'needs implementation' )

    # Implementation optional
    # def onShutdown( self, states ):
    #     return

class SAMInputWidget( SAMWidget ):

    def __init__( self, *a, **k ):
        SAMWidget.__init__( self, *a, **k )
        self.feeds = []

    def addFeed( self, widget ):
        self.feeds.append( widget )
        return self

    def _execute( self, event ):
        outputState = []

        if 0 == len( self.feeds ):
            # We have no piped feeds, so we parse from the current event
            tmpResult = self.execute( event )
            if tmpResult is not None:
                outputState.append( [ tmpResult ] )
        else:
            # We have feeds, so we act as a filter
            for feed in self.feeds:
                newStates = feed._execute( event )
                for tmpState in newStates:
                    for evt in tmpState:
                        # These filters only operate on a single event...
                        tmpResult = self.execute( evt )
                        if None != tmpResult:
                            # The lists here because it is an array of detect and each detect has a list of events.
                            # Since an Input widget only generates an event for simplicity, we wrap it in the SAM format.
                            outputState.append( [ tmpResult ] )

        if self.keys is not None:
            # We have report keys, so we must be a reporting root
            reportKeys = {}
            for k in self.keys:
                newKs = _xm_( outputState, k[ 1 ] )
                if newKs is not None and 0 != len( newKs ):
                    reportKeys.setdefault( k[ 0 ], [] ).extend( newKs )
            return [ outputState, reportKeys ]

        if self.isDebug:
            print( "Input size: %d, output size: %d" % ( len( event ), len( outputState ) ) )

        return outputState

    def execute( self, states ):
        raise Exception( 'needs implementation' )


class StateAnalysisMachine( object ):

    def __init__( self, modules = {} ):
        self.modules = {}
        self.eventId = 0

        for name, m in modules.iteritems():
            instance = eval( '(%s)' % m, globals() )
            if not hasattr( instance, '_execute' ):
                raise Exception( 'invalid root widget: %s' % m )
            self.modules[ name ] = instance

    def execute( self, routing, event ):
        detects = []

        self.eventId += 1
        for name, m in self.modules.iteritems():
            exec_results = m._execute( { 'routing' : routing, 'event' : event, 'event_id' : self.eventId } )

            if exec_results is not None and 0 != len( exec_results ):
                if type( exec_results ) is list:
                    new_detects, keys = exec_results
                else:
                    new_detects = exec_results
                    keys = {}

                keys[ 'BEHAVIOR' ] = [ name ]

                if new_detects is not None and 0 != len( new_detects ):
                    # The new event must be part of this detect, otherwise it's an old one
                    for detect in new_detects:
                        for event in detect:
                            eventIds = _xm_( event, 'event_id' )
                            if self.eventId in eventIds:
                                detects.append( { 'details' : detect, 'module' : name, 'keys' : keys } )
                                break

        return detects
