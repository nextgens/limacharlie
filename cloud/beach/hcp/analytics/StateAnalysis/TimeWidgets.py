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

from ...hcp_helpers import _x_
from ...hcp_helpers import _xm_
from sets import Set

from . import SAMStateWidget
from ...hcp_helpers import allOf, anyOf, anyOfIn


def _isEventCombinationAlreadyReported( newEvent, oldEvents ):
    if anyOf( oldEvents, lambda x: Set( _xm_( newEvent, 'event_id' ) ) == Set( _xm_( x, 'event_id' ) ) ):
        return True
    return False


class TimeCorrelatorWidget( SAMStateWidget ):

    def _findMatches( self, states, matches = [], isPartial = False ):
        results = []

        isNewMatchFound = False
        nextState = states[ 0 ]

        for e in nextState:
            v1 = _x_( e, '?/base.TIMESTAMP' )
            isMatch = True

            if 'within' in self.parameters:
                minTime = v1
                maxTime = v1
                for m in matches:
                    v2 = _x_( m, '?/base.TIMESTAMP' )
                    if v2 < minTime:
                        minTime = int( v2 )
                    if v2 > maxTime:
                        maxTime = int( v2 )
                    if ( maxTime - minTime ) > int( self.parameters[ 'within' ] ):
                        isMatch = False
                        break

            if isMatch:
                tmpMatches = matches[ : ]
                tmpMatches.append( e )
                if 1 < len( states ):
                    results += self._findMatches( states[ 1 : ], tmpMatches, isPartial )
                else:
                    results.append( tmpMatches )
                isNewMatchFound = True

        if not isNewMatchFound and isPartial:
            # We don't need correlation between ALL sources, so we will keep trying even if a source has no hits
            if 1 < len( states ):
                results = self._findMatches( states[ 1 : ], matches[ : ], isPartial )
            else:
                results.append( matches )

        return results

    def execute( self, states ):
        detect = None

        minimum = None
        if 'min' in self.parameters:
            minimum = int( self.parameters[ 'min' ] )

        detect = self._findMatches( states, isPartial = True if None != minimum else False )

        if None != minimum:
            actualDetects = []
            for d in detect:
                if len( d ) >= minimum:
                    if not _isEventCombinationAlreadyReported( d, actualDetects ):
                        actualDetects.append( d )

            detect = actualDetects

        return detect


class TimeBurstWidget( SAMStateWidget ):

    #TODO: we could optimize the hell out of this

    def _getEventsWithin( self, events, seconds, timestamp ):
        matches = []
        for event in events:
            ts = _x_( event, 'event/?/base.TIMESTAMP' )
            if ts is not None:
                if ( ts >= timestamp - seconds ) and ( ts <= timestamp + seconds ):
                   matches.append( event )
        return matches

    def _findMatches( self, states ):
        results = []

        for state in states:
            for event in state:
                ts = _x_( event, 'event/?/base.TIMESTAMP' )
                matches = self._getEventsWithin( state, self.parameters[ 'within' ], ts )
                if len( matches ) >= self.parameters[ 'min' ]:
                    if not _isEventCombinationAlreadyReported( matches, results ):
                        results.append( matches )

        return results

    def execute( self, states ):
        detect = None

        minimum = None
        if 'min' in self.parameters:
            minimum = int( self.parameters[ 'min' ] )

        detect = self._findMatches( states )

        return detect