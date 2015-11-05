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

from . import SAMInputWidget
from . import SAMStateWidget


class ProcessNamedWidget( SAMInputWidget ):
    cache = None
    isPositive = True

    def execute( self, event ):
        detect = None

        # Initialize a quick local cache, list -> Set
        if None == self.cache:
            exes = self.parameters[ 'exe' ]
            if type( exes ) is not list:
                exes = [ self.parameters[ 'exe' ] ]
            exes = [ x.lower() for x in exes ]
            self.cache = Set( exes )
            if 'not' in self.parameters and True == self.parameters[ 'not' ]:
                self.isPositive = False

        processPath = _x_( event, 'notification.NEW_PROCESS/base.FILE_PATH' )

        # If we got a file path in a new process, we try to find the last component after \ or /
        if None != processPath:
            i = processPath.rfind( '\\' )
            if -1 == i:
                i = processPath.rfind( '/' )
            exeName = processPath[ i + 1 : ]

            # If the exe name is in our cache
            if exeName.lower() in self.cache and self.isPositive:
                detect = event
            elif not self.isPositive:
                detect = event

        return detect

class ProcessFullFeedWidget( SAMInputWidget ):

    def execute( self, event ):
        detect = None

        processRec = _x_( event, 'notification.NEW_PROCESS' )

        if None != processRec:
            detect = event

        return detect

class ProcessKillFullFeedWidget( SAMInputWidget ):

    def execute( self, event ):
        detect = None

        processRec = _x_( event, 'notification.TERMINATE_PROCESS' )

        if None != processRec:
            detect = event

        return detect

class ProcessChildrenWidget( SAMStateWidget ):

    def onShutdown( self, states ):
        self.setStates( [ [], [], [] ] )

    def execute( self, states ):
        detect = []

        if 3 == len( states ):
            parents = states[ 0 ]
            fullFeed = states[ 1 ]
            killFeed = states[ 2 ]

            # Process kills, and flush the killFeed
            if 0 != len( killFeed ):
                tmpFull = []
                tmpParent = []
                killPids = _xm_( killFeed, 'notification.TERMINATE_PROCESS/base.PROCESS_ID' )
                for process in fullFeed:
                    fullPids = _xm_( process, 'notification.NEW_PROCESS/base.PROCESS_ID' )
                    isToKill = False
                    for pid in fullPids:
                        if pid in killPids:
                            isToKill = True
                            break
                    if not isToKill:
                        tmpFull.append( process )
                for process in parents:
                    parentPids = _xm_( process, 'notification.NEW_PROCESS/base.PROCESS_ID' )
                    isToKill = False
                    for pid in parentPids:
                        if pid in killPids:
                            isToKill = True
                            break
                    if not isToKill:
                        tmpParent.append( process )

                parents = tmpParent
                fullFeed = tmpFull
                killFeed = []
                self.setStates( [ parents, fullFeed, killFeed ] )

            # Look for relations
            for process in fullFeed:
                isChild = False
                ppid = _xm_( process, 'notification.NEW_PROCESS/base.PARENT_PROCESS_ID' )

                if None == ppid or 0 == len( ppid ):
                    continue

                for e in parents:
                    pid = _xm_( e, 'notification.NEW_PROCESS/base.PROCESS_ID' )

                    if None == pid or 0 == len( pid ):
                        continue

                    isHit = False
                    for p1 in pid:
                        for p2 in ppid:
                            if p1 == p2:
                                isHit = True
                                break
                        if isHit:
                            break

                    if isHit:
                        detect.append( process )

        else:
            raise Exception( 'needs 3 states, [ parents, fullFeed, killFeed ]' )

        return detect

class ProcessDescendantsWidget( SAMStateWidget ):

    def onShutdown( self, states ):
        self.setStates( [ [], [], [] ] )

    def execute( self, states ):
        detect = []

        if 3 == len( states ):
            parents = states[ 0 ]
            fullFeed = states[ 1 ]
            killFeed = states[ 2 ]

            # Process kills, and flush the killFeed
            if 0 != len( killFeed ):
                tmpFull = []
                tmpParent = []

                killPids = _xm_( killFeed, 'notification.TERMINATE_PROCESS/base.PROCESS_ID' )
                for process in fullFeed:
                    fullPids = _xm_( process, 'notification.NEW_PROCESS/base.PROCESS_ID' )
                    isToKill = False
                    for pid in fullPids:
                        if pid in killPids:
                            isToKill = True
                            break
                    if not isToKill:
                        tmpFull.append( process )
                for process in parents:
                    parentPids = _xm_( process, 'notification.NEW_PROCESS/base.PROCESS_ID' )
                    isToKill = False
                    for pid in parentPids:
                        if pid in killPids:
                            isToKill = True
                            break
                    if not isToKill:
                        tmpParent.append( process )

                parents = tmpParent
                fullFeed = tmpFull
                killFeed = []
                self.setStates( [ parents, fullFeed, killFeed ] )

            isProcessedDescendants = True

            while isProcessedDescendants:
                isProcessedDescendants = False

                for processes in fullFeed:
                    for process in processes:
                        isChild = False
                        ppid = _xm_( process, 'notification.NEW_PROCESS/base.PARENT_PROCESS_ID' )
                        tmp_pid = _xm_( process, 'notification.NEW_PROCESS/base.PROCESS_ID' )

                        if None == ppid or None == tmp_pid:
                            continue

                        isHit = False
                        isAlreadyIn = False

                        for e in parents:
                            pid = _xm_( e, 'notification.NEW_PROCESS/base.PROCESS_ID' )

                            if None == pid:
                                continue

                            for p1 in pid:
                                for p2 in ppid:
                                    if p1 == p2:
                                        isHit = True
                                        break
                                if isHit:
                                    break

                                #process[ 'parent' ] = e

                            for p1 in pid:
                                for p2 in tmp_pid:
                                    if p1 == p2:
                                        isAlreadyIn = True
                                        break
                                if isAlreadyIn:
                                    break

                            if isAlreadyIn:
                                break

                        if isHit:
                            if not isAlreadyIn:
                                # Now we do something naughty / feature and add back the hit to the parent states
                                # so that we hit on those in the future as well...
                                parents.append( process )
                                isProcessedDescendants = True
                            detect.append( processes )

        return detect