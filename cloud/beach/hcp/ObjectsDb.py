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
from sets import Set
import hashlib
import base64
import uuid
import msgpack
import sys
import time
import re
AgentId = Actor.importLib( 'hcp_helpers', 'AgentId' )
chunks = Actor.importLib( 'hcp_helpers', 'chunks' )
tsToTime = Actor.importLib( 'hcp_helpers', 'tsToTime' )
timeToTs = Actor.importLib( 'hcp_helpers', 'timeToTs' )
CassDb = Actor.importLib( 'hcp_databases', 'CassDb' )
CassPool = Actor.importLib( 'hcp_databases', 'CassPool' )

def ObjectNormalForm( objName, objType, isCaseSensitive = False ):
    caseSensitiveTypes = ( ObjectTypes.AUTORUNS,
                           ObjectTypes.CMD_LINE,
                           ObjectTypes.FILE_NAME,
                           ObjectTypes.FILE_PATH,
                           ObjectTypes.HANDLE_NAME,
                           ObjectTypes.MODULE_NAME,
                           ObjectTypes.PACKAGE,
                           ObjectTypes.PROCESS_NAME,
                           ObjectTypes.SERVICE_NAME )
    if ObjectTypes.FILE_HASH == objType:
        try:
            objName.decode( 'ascii' )
        except:
            objName = objName.encode( 'hex' )
    else:
        try:
            objName = unicode( objName )
            if not isCaseSensitive or objType not in caseSensitiveTypes:
                objName = objName.lower()
            objName = objName.encode( 'utf-8' )
        except:
            objName = base64.b64encode( objName )
        if ObjectTypes.FILE_PATH == objType:
            objName = NormalizePath( objName, isCaseSensitive = isCaseSensitive )
    return objName

def ObjectKey( objName, objType ):
    k = hashlib.sha256( '%s/%s' % ( objName, objType ) ).hexdigest()
    return k

def RelationNameFromId( pId, cId ):
    v = '%s/%s' % ( pId, cId )
    return v

def RelationName( pName, pType, cName, cType ):
    v = RelationNameFromId( ObjectKey( pName, pType ), ObjectKey( cName, cType ) )
    return v

def NormalizePath( path, isCaseSensitive = False ):
    for n in ( ( r'.:\\users\\.+?(\\.+)', r'%USERPROFILE%\1' ),
               ( r'.:\\documents and settings\\.+?(\\.+)', r'%USERPROFILE%\1' ),
               ( r'/Users/.+?(/.+)', r'$HOME\1' ),
               ( r'/home/.+?(/.+)', r'$HOME\1' ) ):
        path = re.sub( n[ 0 ], n[ 1 ], path, flags = ( 0 if isCaseSensitive else re.IGNORECASE ) )
    return path

class ObjectTypes (object):
    RELATION = 0
    FILE_PATH = 1
    FILE_NAME = 2
    PROCESS_NAME = 3
    MODULE_NAME = 4
    MODULE_SIZE = 5
    FILE_HASH = 6
    HANDLE_NAME = 7
    SERVICE_NAME = 8
    CMD_LINE = 9
    MEM_HEADER_HASH = 10
    PORT = 11
    THREADS = 12
    AUTORUNS = 13
    DOMAIN_NAME = 14
    PACKAGE = 15
    STRING = 16
    IP_ADDRESS = 17

    tup = ( ( 'RELATION', 0 ),
            ( 'FILE_PATH', 1 ),
            ( 'FILE_NAME', 2 ),
            ( 'PROCESS_NAME', 3 ),
            ( 'MODULE_NAME', 4 ),
            ( 'MODULE_SIZE', 5 ),
            ( 'FILE_HASH', 6 ),
            ( 'HANDLE_NAME', 7 ),
            ( 'SERVICE_NAME', 8 ),
            ( 'CMD_LINE', 9 ),
            ( 'MEM_HEADER_HASH', 10 ),
            ( 'PORT', 11 ),
            ( 'THREADS', 12 ),
            ( 'AUTORUNS', 13 ),
            ( 'DOMAIN_NAME', 14 ),
            ( 'PACKAGE', 15 ),
            ( 'STRING', 16 ),
            ( 'IP_ADDRESS', 17 ) )

    forward = { 'RELATION': 0,
                'FILE_PATH': 1,
                'FILE_NAME': 2,
                'PROCESS_NAME': 3,
                'MODULE_NAME': 4,
                'MODULE_SIZE': 5,
                'FILE_HASH': 6,
                'HANDLE_NAME': 7,
                'SERVICE_NAME': 8,
                'CMD_LINE': 9,
                'MEM_HEADER_HASH': 10,
                'PORT': 11,
                'THREADS': 12,
                'AUTORUNS': 13,
                'DOMAIN_NAME': 14,
                'PACKAGE': 15,
                'STRING': 16,
                'IP_ADDRESS': 17 }

    rev = { 0 : 'RELATION',
            1 : 'FILE_PATH',
            2 : 'FILE_NAME',
            3 : 'PROCESS_NAME',
            4 : 'MODULE_NAME',
            5 : 'MODULE_SIZE',
            6 : 'FILE_HASH',
            7 : 'HANDLE_NAME',
            8 : 'SERVICE_NAME',
            9 : 'CMD_LINE',
            10 : 'MEM_HEADER_HASH',
            11 : 'PORT',
            12 : 'THREADS',
            13 : 'AUTORUNS',
            14 : 'DOMAIN_NAME',
            15 : 'PACKAGE',
            16 : 'STRING',
            17 : 'IP_ADDRESS' }

def dbgprint( s ):
    sys.stderr.write( s + "\n" )
    sys.stderr.flush()

class HostObjects( object ):
    _db = None
    _idRe = re.compile( '^[a-zA-Z0-9]{64}$' )
    _queryChunks = 200

    @classmethod
    def setDatabase( cls, url, backoffConsistency = True ):
        cls._db = CassDb( url, 'hcp_analytics', quorum = False, backoffConsistency = backoffConsistency )

    @classmethod
    def closeDatabase( cls ):
        cls._db.shutdown()

    @classmethod
    def onHosts( cls, hosts, types = [], within = None ):
        if within is not None:
            within = int( time.time() ) - int( within )

        if type( hosts ) is not list and type( hosts ) is not tuple:
            hosts = ( hosts, )

        if type( types ) is not list and type( types ) is not tuple:
            types = ( types, )

        def thisGen():
            for host in hosts:
                if 0 == len( types ):
                    for row in cls._db.execute( 'SELECT id, last FROM loc WHERE aid = %s', ( AgentId( host ).invariableToString(), ) ):
                        if within is None or int( time.mktime( row[ 1 ].timetuple() ) ) >= within:
                            yield row[ 0 ]
                else:
                    rows = []
                    for t in types:
                        for row in cls._db.execute( 'SELECT id, last FROM loc WHERE aid = %s AND otype = %s', ( AgentId( host ).invariableToString(), cls._castType( t ) ) ):
                            if within is None or int( time.mktime( row[ 1 ].timetuple() ) ) >= within:
                                yield row[ 0 ]

        return cls( thisGen() )

    @classmethod
    def ofTypes( cls, types ):
        if type( types ) is not list and type( types ) is not tuple:
            types = ( types, )

        def thisGen():
            for t in types:
                # Because they are sharded in 256 shards and because the actual records' primary key includes an aid
                # there will be duplicates between shards and even within a single shard. So to provide a unified
                # view we need to summarize on a per-type basis.
                ids = Set()
                for d in range( 0, 256 ):
                    for row in cls._db.execute( 'SELECT id FROM loc_by_type WHERE d256 = %s AND otype = %s', ( d, cls._castType( t ) ) ):
                        ids.add( row[ 0 ] )
                for id in ids:
                    yield id

        return cls( thisGen() )

    @classmethod
    def matching( cls, ofType, withParents, withChildren ):
        if type( ofType ) is str or type( ofType ) is unicode:
            ofType = ObjectTypes.forward[ ofType ]

        def thisGen():
            ids = Set()
            for cid in withChildren:
                tmp = [ x[ 0 ] for x in cls._db.execute( 'SELECT pid FROM rel_man_child WHERE childKey = %s AND ptype = %s', ( cid, ofType ) ) ]
                if 0 == len( ids ):
                    ids = Set( tmp )
                else:
                    ids = ids.intersection( tmp )

            for pid in withParents:
                tmp = [ x[ 0 ] for x in cls._db.execute( 'SELECT cid FROM rel_man_parent WHERE parentKey = %s AND ctype = %s', ( pid, ofType ) ) ]
                if 0 == len( ids ):
                    ids = Set( tmp )
                else:
                    ids = ids.intersection( tmp )

            for id in ids:
                yield id

        return cls( thisGen() )

    @classmethod
    def named( cls, named ):

        def thisGen():
            hasResult = False

            isDoCaseSensitiveSearch = False
            # If there are upper cases in the name, let's search for it in
            # both case sensisitve and insensitive.
            if named != named.lower():
                isDoCaseSensitiveSearch = True

            for x in cls._db.execute( 'SELECT id FROM obj_name WHERE obj = %s', ( ObjectNormalForm( named, None ), ) ):
                hasResult = True
                yield x[ 0 ]

            # If we got no result, check for a file path since it has more normalization
            if not hasResult:
                for x in cls._db.execute( 'SELECT id FROM obj_name WHERE obj = %s', ( ObjectNormalForm( named, ObjectTypes.FILE_PATH ), ) ):
                    yield x[ 0 ]

            if isDoCaseSensitiveSearch:
                for x in cls._db.execute( 'SELECT id FROM obj_name WHERE obj = %s', ( ObjectNormalForm( named, None, isCaseSensitive = True ), ) ):
                    hasResult = True
                    yield x[ 0 ]

                # If we got no result, check for a file path since it has more normalization
                if not hasResult:
                    for x in cls._db.execute( 'SELECT id FROM obj_name WHERE obj = %s', ( ObjectNormalForm( named, ObjectTypes.FILE_PATH, isCaseSensitive = True ), ) ):
                        yield x[ 0 ]


        return cls( thisGen() )

    @classmethod
    def _castType( cls, t ):
        if type( t ) is str or type( t ) is unicode:
            return ObjectTypes.forward[ t ]
        else:
            return int( t )

    @classmethod
    def _castId( cls, id ):
        id = str( id )
        if cls._idRe.match( id ):
            return id.lower()
        else:
            return None

    def __init__( self, ids = [] ):
        if not hasattr( ids, '__iter__' ):
            ids = [ ids ]
        self._ids = ids

    def __iter__( self ):
        return self._ids.__iter__()

    def next( self ):
        return self._ids.next()

    def info( self ):

        for ids in chunks( self._ids, self._queryChunks ):
            try:
                for row in self._db.execute( 'SELECT id, obj, otype FROM obj_man WHERE id IN ( \'%s\' )' % '\',\''.join( ids ) ):
                    yield ( row[ 0 ], row[ 1 ], row[ 2 ] )
            except:
                pass


    def locs( self, within = None, isLocalCloudOnly = False ):
        if within is not None:
            within = int( time.time() ) - int( within )

        # There are currently performance issues with filtering by time within in this query.
        within = None

        for ids in chunks( self._ids, self._queryChunks ):
            for row in self._db.execute( 'SELECT id, aid , last FROM loc_by_id WHERE id IN ( \'%s\' )' % '\',\''.join( ids ) ):
                ts = int( time.mktime( row[ 2 ].timetuple() ) ) if row[ 2 ] is not None else 0
                if within is None:
                    yield ( row[ 0 ], row[ 1 ], ts )
                else:
                    if ts >= within:
                        yield ( row[ 0 ], row[ 1 ], ts )
            if within is None and isLocalCloudOnly is not True:
                ts = 0
                for row in self._db.execute( 'SELECT id, plat, nloc FROM ref_loc WHERE id IN ( \'%s\' )' % '\',\''.join( ids ) ):
                    for i in range( 1, row[ 2 ] if row[ 2 ] < 100000 else 100000 ):
                        yield ( row[ 0 ], 'ff.ff.%x.%x' % ( i, row[ 1 ] << 4 ), ts )

    def children( self, types = None ):
        withType = ''
        if types is not None:
            withType = ' AND ctype = %d' % int( self._castType( types ) )

        def thisGen():
            for ids in chunks( self._ids, self._queryChunks ):
                for row in self._db.execute( 'SELECT cid FROM rel_man_parent WHERE parentkey IN ( \'%s\' )%s' % ( '\',\''.join( ids ), withType ) ):
                    yield row[ 0 ]

        return type(self)( thisGen() )

    def childrenRelations( self, types = None ):

        def thisGen():
            for id in self._ids:
                for child in type(self)( id ).children( types ):
                    yield ObjectKey( RelationNameFromId( id, child ), ObjectTypes.RELATION )

        return type(self)( thisGen() )

    def parents( self, types = None ):
        withType = ''
        if types is not None:
            withType = ' AND ptype = %d' % int( self._castType( types ) )

        def thisGen():
            for ids in chunks( self._ids, self._queryChunks ):
                for row in self._db.execute( 'SELECT pid FROM rel_man_child WHERE childkey IN ( \'%s\' )%s' % ( '\',\''.join( ids ), withType ) ):
                    yield row[ 0 ]

        return type(self)( thisGen() )

    def parentsRelations( self, types = None ):

        def thisGen():
            for id in self._ids:
                for parent in type(self)( id ).parents( types ):
                    yield ObjectKey( RelationNameFromId( parent, id ), ObjectTypes.RELATION )

        return type(self)( thisGen() )

    def lastSeen( self, forAgents = None ):
        if forAgents is not None and type( forAgents ) is not tuple and type( forAgents ) is not list:
            forAgents = ( forAgents, )
        forAgents = [ x.invariableToString() for x in forAgents ]
        for ids in chunks( self._ids, self._queryChunks ):
            for row in self._db.execute( 'SELECT id, aid, last FROM loc_by_id WHERE id IN ( \'%s\' )' % '\',\''.join( ids ) ):
                if forAgents is not None:
                    if row[ 1 ] not in forAgents:
                        continue
                yield ( row[ 0 ], row[ 1 ], row[ 2 ] )

class Host( object ):

    _be = None
    _db = None

    @classmethod
    def setDatabase( cls, beInstance, cassUrl, backoffConsistency = True ):
        cls._be = beInstance
        cls._db = CassDb( cassUrl, 'hcp_analytics', quorum = True, backoffConsistency = backoffConsistency )

    @classmethod
    def closeDatabase( cls ):
        cls._db.shutdown()

    @classmethod
    def getHostsMatching( cls, mask = None, hostname = None ):
        col = []
        agents = cls._be.hcp_getAgentStates( aid = mask, hostname = hostname )
        if agents.isSuccess and 'agents' in agents.data:
            col = [ Host( x ) for x in agents.data[ 'agents' ].keys() ]
        return col

    @classmethod
    def getSpecificEvent( self, id ):
        record = None

        event = self._db.getOne( 'SELECT agentid, event FROM events WHERE eventid = %s', ( id, ) )
        if event is not None:
            record = ( id, event[ 0 ], event[ 1 ] )

        return record

    def __init__( self, agentid ):
        if type( agentid  ) is not AgentId:
            agentid = AgentId( agentid )
        self.aid = agentid

    def __str__( self ):
        return str( self.aid )

    def isOnline( self ):
        isOnline = False
        aid = str( self.aid )
        info = self._be.hcp_getAgentStates( aid = aid )
        if info.isSuccess and 'agents' in info.data and 0 != len( info.data[ 'agents' ] ):
            info = info.data[ 'agents' ].values()[ 0 ]
            ts = timeToTs( info[ 'date_last_seen' ] )
            if ts >= time.time() - ( 60 * 10 ):
                isOnline = True

        return isOnline

    def getHostName( self ):
        hostname = None

        aid = str( self.aid )
        info = self._be.hcp_getAgentStates( aid = aid )
        if info.isSuccess and 'agents' in info.data and 0 != len( info.data[ 'agents' ] ):
            info = info.data[ 'agents' ].values()[ 0 ]
            hostname = info[ 'last_hostname' ]

        return hostname

    def getStatusHistory( self, within = None ):
        statuses = []

        whereTs = ''
        filters = []
        filters.append( self.aid.invariableToString() )
        if within is not None:
            ts = tsToTime( int( within ) )
            whereTs = ' AND ts >= minTimeuuid(%s)'
            filters.append( ts )

        results = self._db.execute( 'SELECT unixTimestampOf( ts ) FROM timeline WHERE agentid = %%s AND eventtype = \'hbs.NOTIFICATION_STARTING_UP\'%s' % whereTs, filters )
        if results is not None:
            for result in results:
                statuses.append( ( result[ 0 ], True ) )

        results = self._db.execute( 'SELECT unixTimestampOf( ts ) FROM timeline WHERE agentid = %%s AND eventtype = \'hbs.NOTIFICATION_SHUTTING_DOWN\'%s' % whereTs, filters )
        if results is not None:
            for result in results:
                statuses.append( ( result[ 0 ], False ) )

        return statuses

    def getEvents( self, before = None, after = None, limit = None, ofTypes = None, isIncludeContent = False ):
        events = []

        filters = []
        filterValues = []

        filters.append( 'agentid = %s' )
        filterValues.append( self.aid.invariableToString() )

        if before is not None and before != '':
            filters.append( 'ts <= maxTimeuuid(%s)' )
            filterValues.append( tsToTime( before ) )

        if after is not None and after != '':
            filters.append( 'ts >= minTimeuuid(%s)' )
            filterValues.append( tsToTime( after ) )

        if ofTypes is not None:
            if type( ofTypes ) is not tuple and type( ofTypes ) is not list:
                ofTypes = ( ofTypes, )

        if limit is not None:
            limit = 'LIMIT %d' % int( limit )
        else:
            limit = ''

        def thisGen():
            if ofTypes is None:
                for row in self._db.execute( 'SELECT unixTimestampOf( ts ), eventtype, eventid FROM timeline WHERE %s%s' % ( ' AND '.join( filters ), limit ), filterValues ):
                    record = ( row[ 0 ], row[ 1 ], row[ 2 ] )
                    if isIncludeContent:
                        event = self._db.getOne( 'SELECT event FROM events WHERE eventid = %s', ( record[ 2 ], ) )
                        if event is not None:
                            record = ( record[ 0 ], record[ 1 ], record[ 2 ], event[ 0 ] )

                    yield record
            else:
                for t in ofTypes:
                    tmp_filters = [ 'eventtype = %s' ]
                    tmp_filters.extend( filters )
                    tmp_values = [ t ]
                    tmp_values.extend( filterValues )
                    for row in self._db.execute( 'SELECT unixTimestampOf( ts ), eventtype, eventid FROM timeline WHERE %s%s' % ( ' AND '.join( tmp_filters ), limit ), tmp_values ):
                        record = ( row[ 0 ], row[ 1 ], row[ 2 ] )
                        if isIncludeContent:
                            event = self._db.getOne( 'SELECT event FROM events WHERE eventid = %s', ( record[ 2 ], ) )
                            if event is not None:
                                record = ( record[ 0 ], record[ 1 ], record[ 2 ], event[ 0 ] )
                            else:
                                record = ( record[ 0 ], record[ 1 ], record[ 2 ], None )

                        yield record

        return thisGen()

    def lastSeen( self ):
        last = None
        record = self._db.getOne( 'SELECT last from recentlyActive WHERE agentid = %s', ( self.aid.invariableToString(), ) )
        if record is not None:
            last = int( time.mktime( record[ 0 ].timetuple() ) )

        return last

    def lastEvents( self ):
        events = []

        for row in self._db.execute( 'SELECT type, id FROM last_events WHERE agentid = %s', ( self.aid.invariableToString(), ) ):
            events.append( { 'name' : row[ 0 ], 'id' : row[ 1 ] } )

        return events

class FluxEvent( object ):
    @classmethod
    def decode( cls, event ):
        try:
            event = msgpack.unpackb( base64.b64decode( event ), use_list = True )[ 'event' ]
            cls._dataToUtf8( event )
        except:
            event = None
        return event

    @classmethod
    def _dataToUtf8( cls, node ):
        newVal = None

        if type( node ) is dict:
            for k, n in node.iteritems():
                if 'base.HASH' == k or str( k ).endswith( '_HASH' ):
                    node[ k ] = n.encode( 'hex' )
                else:
                    tmp = cls._dataToUtf8( n )
                    if tmp is not None:
                        node[ k ] = tmp
        elif type( node ) is list or type( node ) is tuple:
            for index, n in enumerate( node ):
                tmp = cls._dataToUtf8( n )
                if tmp is not None:
                    node[ index ] = tmp
        else:
            # This is a leaf
            if type( node ) is str:
                try:
                    newVal = node.decode( 'utf-8' )
                except:
                    newVal = None
        return newVal
