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
#from analytics.StateAnalysis import StateAnalysisMachine

def GenerateDetect( objects = [], relations = [], desc = None, mtd = {} ):
    d = {}

    if 0 != len( objects ):
        d[ 'obj' ] = objects
    if 0 != len( relations ):
        d[ 'rel' ] = relations
    if desc is not None:
        d[ 'desc' ] = desc
    if 0 != len( mtd ):
        d[ 'mtd' ] = mtd

    return d

def GenerateDetectReport( msg, cat, reportId, detects = [] ):
    if 0 == len( detects ):
        return None
    else:
        return { 'msg' : msg, 'cat' : cat, 'detects' : detects, 'report_id' : reportId }



class StatelessActor ( Actor ):
    def init( self, parameters ):
        if not hasattr( self, 'process' ):
            raise Exception( 'Stateless Actor has no "process" function' )
        self._reporting = self.getActorHandle( 'analytics/report' )
        self.handle( 'process', self._process )

    def newDetect( self, objects = [], relations = [], desc = None, mtd = {} ):
        d = {}

        if 0 != len( objects ):
            d[ 'obj' ] = objects
        if 0 != len( relations ):
            d[ 'rel' ] = relations
        if desc is not None:
            d[ 'desc' ] = desc
        if 0 != len( mtd ):
            d[ 'mtd' ] = mtd

        return d

    def _process( self, msg ):
        detects = self.process( msg )

        if 0 != len( detects ):
            routing, event, mtd = msg.data
            self._reporting.shoot( 'report', GenerateDetectReport( ( msg, ),
                                                                   str( type( self ) ),
                                                                   routing[ 'event_id' ],
                                                                   detects ) )

        return ( True, )


# class StatefulActor ( Actor ):
#     def init( self, parameters ):
#         if not hasattr( self, 'processDetects' ):
#             raise Exception( 'Stateful Actor has no "processDetects" function' )
#         if not hasattr( self, 'machines' ):
#             raise Exception( 'Stateful Actor has no associated detection machines' )
#         self._machine = StateAnalysisMachine( self.machines )
#         self._reporting = self.getActorHandle( 'analytics/report' )
#         self.handle( 'process', self._process )
#
#     def newDetect( self, objects = [], relations = [], desc = None, mtd = {} ):
#         return GenerateDetect( objects = objects, relations = relations, desc = desc, mtd = mtd )
#
#     def _process( self, msg ):
#         routing, event, mtd = msg.data
#
#         detects = self._machine.execute( routing, event )
#
#         if detects is not None and 0 != len( detects ):
#             for detect in detects:
#                 detect = self.processDetects( detect )
#                 if detect is not None:
#                     self._reporting.shoot( 'report', GenerateDetectReport( ( msg, ),
#                                                                            str( type( self ) ),
#                                                                            routing[ 'event_id' ],
#                                                                            detects ) )
#
#         return ( True, )