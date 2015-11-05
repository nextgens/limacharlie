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
import time
import time_uuid
import uuid
import base64
import msgpack
import datetime
import random
from sets import Set
CassDb = Actor.importLib( '../hcp_databases', 'CassDb' )
CassPool = Actor.importLib( '../hcp_databases', 'CassPool' )
AgentId = Actor.importLib( '../hcp_helpers', 'AgentId' )
_x_ = Actor.importLib( '../hcp_helpers', '_x_' )
_xm_ = Actor.importLib( '../hcp_helpers', '_xm_' )
ObjectTypes = Actor.importLib( '../ObjectsDb', 'ObjectTypes' )
RelationName = Actor.importLib( '../ObjectsDb', 'RelationName' )
ObjectKey = Actor.importLib( '../ObjectsDb', 'ObjectKey' )

class AnalyticsModeling( Actor ):
    def init( self, parameters ):
        self._db = CassDb( parameters[ 'db' ], 'hcp_analytics', consistencyOne = True )
        self.db = CassPool( self._db,
                            rate_limit_per_sec = parameters[ 'rate_limit_per_sec' ],
                            maxConcurrent = parameters[ 'max_concurrent' ],
                            blockOnQueueSize = parameters[ 'block_on_queue_size' ] )
        #self.db = self._db

        self.ignored_objects = [ ObjectTypes.STRING,
                                 ObjectTypes.IP_ADDRESS,
                                 ObjectTypes.MODULE_SIZE,
                                 ObjectTypes.STRING,
                                 ObjectTypes.THREADS,
                                 ObjectTypes.MEM_HEADER_HASH ]

        self.temporary_objects = [ ObjectTypes.CMD_LINE,
                                   ObjectTypes.DOMAIN_NAME,
                                   ObjectTypes.PORT ]

        self.statements = {}
        self.statements[ 'events' ] = self.db.prepare( 'INSERT INTO events ( eventid, event, agentid ) VALUES ( ?, ?, ? ) USING TTL %d' % ( 60 * 60 * 24 * 7 * 2 ) )
        self.statements[ 'timeline' ] = self.db.prepare( 'INSERT INTO timeline ( agentid, ts, eventid, eventtype ) VALUES ( ?, ?, ?, ? ) USING TTL %d' % ( 60 * 60 * 24 * 7 * 2 ) )
        self.statements[ 'recent' ] = self.db.prepare( 'UPDATE recentlyActive USING TTL %d SET last = dateOf( now() ) WHERE agentid = ?' % ( 60 * 60 * 24 * 14 ) )
        self.statements[ 'last' ] = self.db.prepare( 'UPDATE last_events USING TTL %d SET id = ? WHERE agentid = ? AND type = ?' % ( 60 * 60 * 24 * 30 * 1 ) )
        self.statements[ 'investigation' ] = self.db.prepare( 'INSERT INTO investigation_data ( invid, ts, eid, etype ) VALUES ( ?, ?, ?, ? ) USING TTL %d' % ( 60 * 60 * 24 * 30 * 1 ) )

        self.statements[ 'rel_batch_parent' ] = self.db.prepare( '''INSERT INTO rel_man_parent ( parentkey, ctype, cid ) VALUES ( ?, ?, ? ) USING TTL 31536000;''' )
        self.statements[ 'rel_batch_child' ] = self.db.prepare( '''INSERT INTO rel_man_child ( childkey, ptype, pid ) VALUES ( ?, ?, ? ) USING TTL 31536000;''' )

        self.statements[ 'rel_batch_tmp_parent' ] = self.db.prepare( '''INSERT INTO rel_man_parent ( parentkey, ctype, cid ) VALUES ( ?, ?, ? ) USING TTL 15552000;''' )
        self.statements[ 'rel_batch_tmp_child' ] = self.db.prepare( '''INSERT INTO rel_man_child ( childkey, ptype, pid ) VALUES ( ?, ?, ? ) USING TTL 15552000;''' )

        self.statements[ 'obj_batch_man' ] = self.db.prepare( '''INSERT INTO obj_man ( id, obj, otype ) VALUES ( ?, ?, ? ) USING TTL 63072000;''' )
        self.statements[ 'obj_batch_name' ] = self.db.prepare( '''INSERT INTO obj_name ( obj, id ) VALUES ( ?, ? ) USING TTL 63072000;''' )
        self.statements[ 'obj_batch_loc' ] = self.db.prepare( '''UPDATE loc USING TTL 63072000 SET last = ? WHERE aid = ? AND otype = ? AND id = ?;''' )
        self.statements[ 'obj_batch_id' ] = self.db.prepare( '''INSERT INTO loc_by_id ( id, aid, last ) VALUES ( ?, ?, ? ) USING TTL 63072000;''' )
        self.statements[ 'obj_batch_type' ] = self.db.prepare( '''INSERT INTO loc_by_type ( d256, otype, id, aid ) VALUES ( ?, ?, ?, ? ) USING TTL 259200;''' )

        self.statements[ 'obj_batch_tmp_man' ] = self.db.prepare( '''INSERT INTO obj_man ( id, obj, otype ) VALUES ( ?, ?, ? ) USING TTL 15552000;''' )
        self.statements[ 'obj_batch_tmp_name' ] = self.db.prepare( '''INSERT INTO obj_name ( obj, id ) VALUES ( ?, ? ) USING TTL 15552000;''' )
        self.statements[ 'obj_batch_tmp_loc' ] = self.db.prepare( '''UPDATE loc USING TTL 15552000 SET last = ? WHERE aid = ? AND otype = ? AND id = ?;''' )
        self.statements[ 'obj_batch_tmp_id' ] = self.db.prepare( '''INSERT INTO loc_by_id ( id, aid, last ) VALUES ( ?, ?, ? ) USING TTL 15552000;''' )
        self.statements[ 'obj_batch_tmp_type' ] = self.db.prepare( '''INSERT INTO loc_by_type ( d256, otype, id, aid ) VALUES ( ?, ?, ?, ? ) USING TTL 259200;''' )

        for statement in self.statements.values():
            statement.consistency_level = CassDb.CL_Ingest

        self.db.start()
        self.handle( 'analyze', self.analyze )

    def deinit( self ):
        self.db.stop()
        self._db.shutdown()

    def _ingestObjects( self, aid, ts, objects, relations ):
        ts = datetime.datetime.fromtimestamp( ts )

        for relType, relVals in relations.iteritems():
            for relVal in relVals:
                objects.setdefault( ObjectTypes.RELATION, [] ).append( RelationName( relVal[ 0 ],
                                                                                     relType[ 0 ],
                                                                                     relVal[ 1 ],
                                                                                     relType[ 1 ] ) )

                if relType[ 0 ] in self.temporary_objects or relType[ 1 ] in self.temporary_objects:
                    stmt1 = self.statements[ 'rel_batch_tmp_parent' ]
                    stmt2 = self.statements[ 'rel_batch_tmp_child' ]
                else:
                    stmt1 = self.statements[ 'rel_batch_parent' ]
                    stmt2 = self.statements[ 'rel_batch_child' ]

                self.db.execute_async( stmt1.bind( ( ObjectKey( relVal[ 0 ], relType[ 0 ] ),
                                                     relType[ 1 ],
                                                     ObjectKey( relVal[ 1 ], relType[ 1 ] ) ) ) )

                self.db.execute_async( stmt2.bind( ( ObjectKey( relVal[ 1 ], relType[ 1 ] ),
                                                     relType[ 0 ],
                                                     ObjectKey( relVal[ 0 ], relType[ 0 ] ) ) ) )

        for objType, objVals in objects.iteritems():
            for objVal in objVals:
                k = ObjectKey( objVal, objType )

                if objType in self.temporary_objects:
                    stmt1 = self.statements[ 'obj_batch_tmp_man' ]
                    stmt2 = self.statements[ 'obj_batch_tmp_name' ]
                    stmt3 = self.statements[ 'obj_batch_tmp_loc' ]
                    stmt4 = self.statements[ 'obj_batch_tmp_id' ]
                    stmt5 = self.statements[ 'obj_batch_tmp_type' ]
                else:
                    stmt1 = self.statements[ 'obj_batch_man' ]
                    stmt2 = self.statements[ 'obj_batch_name' ]
                    stmt3 = self.statements[ 'obj_batch_loc' ]
                    stmt4 = self.statements[ 'obj_batch_id' ]
                    stmt5 = self.statements[ 'obj_batch_type' ]

                self.db.execute_async( stmt1.bind( ( k, objVal, objType ) ) )
                self.db.execute_async( stmt2.bind( ( objVal, k ) ) )
                self.db.execute_async( stmt3.bind( ( ts, aid, objType, k ) ) )
                self.db.execute_async( stmt4.bind( ( k, aid, ts ) ) )
                self.db.execute_async( stmt5.bind( ( random.randint( 0, 256 ), objType, k, aid ) ) )


    def analyze( self, msg ):
        routing, event, mtd = msg.data

        self.log( 'storing new event' )

        agent = AgentId( routing[ 'agentid' ] )
        aid = agent.invariableToString()
        ts = _x_( event, '?/base.TIMESTAMP' )

        if ts is None or ts > ( 2 * time.time() ):
            ts = _x_( event, 'base.TIMESTAMP' )
            if ts is None:
                ts = time_uuid.utctime()
            else:
                ts = int( ts )

        eid = routing[ 'event_id' ]

        self.db.execute_async( self.statements[ 'events' ].bind( ( eid,
                                                                   base64.b64encode( msgpack.packb( { 'routing' : routing, 'event' : event } ) ),
                                                                   aid ) ) )

        self.db.execute_async( self.statements[ 'timeline' ].bind( ( aid,
                                                                     time_uuid.TimeUUID.with_timestamp( ts ),
                                                                     eid,
                                                                     routing[ 'event_type' ] ) ) )

        self.db.execute_async( self.statements[ 'recent' ].bind( ( aid, ) ) )

        self.db.execute_async( self.statements[ 'last' ].bind( ( eid,
                                                                 aid,
                                                                 routing[ 'event_type' ] ) ) )

        inv_id = _x_( event, '?/hbs.INVESTIGATION_ID' )
        if inv_id is not None and inv_id != '':
            self.db.execute_async( self.statements[ 'investigation' ].bind( ( inv_id.split( '/' )[ 0 ],
                                                                              time_uuid.TimeUUID.with_timestamp( ts ),
                                                                              eid,
                                                                              routing[ 'event_type' ] ) ) )
        self.log( 'storing objects' )
        new_objects = mtd[ 'obj' ]
        new_relations = mtd[ 'rel' ]

        for ignored in self.ignored_objects:
            if ignored in new_objects:
                del( new_objects[ ignored ] )
            for k in new_relations.keys():
                if ignored in k:
                    del( new_relations[ k ] )

        if 0 != len( new_objects ) or 0 != len( new_relations ):
            self._ingestObjects( aid, ts, new_objects, new_relations )
        self.log( 'finished storing objects: %s / %s' % ( len( new_objects ), len( new_relations )) )
        return ( True, )