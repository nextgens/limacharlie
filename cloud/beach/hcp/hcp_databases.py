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
import syslog
from beach.actor import Actor
import traceback
synchronized = Actor.importLib( 'hcp_helpers', 'synchronized' )
from collections import deque
import gevent


try:
    from cassandra.cluster import Cluster
    import traceback
    from cassandra.query import SimpleStatement
    from cassandra import ConsistencyLevel
except:
    pass

try:
    import MySQLdb
    import MySQLdb.cursors
except:
    pass

class HcpDb( object ):
    def __init__( self, uri, dbname, name, password, isConnectRightAway = True ):
        self.hDb = None
        self.hCur = None

        self.cred_dnname = None
        self.cred_uri = None
        self.cred_name = None
        self.cred_password = None

        self.last_error = None

        self.last_activity = 0
        self.connection_timeout = (60*60)

        self.cred_dbname = dbname
        self.cred_uri = uri
        self.cred_name = name
        self.cred_password = password
        self.isInTransaction = False

        if isConnectRightAway:
            self.connect()

    def connect( self ):
        isSuccess = False

        try:
            self.hDb = MySQLdb.connect( host = self.cred_uri, user = self.cred_name, passwd = self.cred_password, db = self.cred_dbname )
            self.hCur = self.hDb.cursor( cursorclass = MySQLdb.cursors.DictCursor )
            isSuccess = True
            self.last_activity = time.time()
        except:
            self.last_error = traceback.format_exc()
            isSuccess = False

        return isSuccess

    def disconnect( self ):
        isSuccess = False

        try:
            self.hCur.close()
            self.hDb.close()
            isSuccess = True
        except:
            self.last_error = traceback.format_exc()
            isSuccess = False

        return isSuccess

    def execute( self, query, params = {} ):
        res = False

        if ( self.last_activity + self.connection_timeout ) < time.time():
            self.disconnect()
            self.connect()

        # Get a new DB snapshot from our query unless we're in a transaction
        if not self.isInTransaction:
            self.commit()

        try:
            self.hCur.execute( query, params )
            self.last_error = None
            res = True
            self.last_activity = time.time()
        except:
            self.last_error = traceback.format_exc()
            res = False

        if not self.isInTransaction:
            self.commit()

        return res

    def query( self, query, params = {} ):
        res = False

        if ( self.last_activity + self.connection_timeout ) < time.time():
            self.disconnect()
            self.connect()

        # Get a new DB snapshot from our query unless we're in a transaction
        if not self.isInTransaction:
            self.commit()

        try:
            self.hCur.execute( query, params )

            res = self.hCur.fetchall()

            self.last_error = None
            self.last_activity = time.time()
        except:
            self.last_error = traceback.format_exc()
            res = False

        return res

    def queryOne( self, query, params = {} ):
        res = self.query( query, params )
        if res is not False and 0 != len( res ):
            return res[ 0 ]
        else:
            return False

    def commit( self ):
        isSuccess = False

        try:
            self.isInTransaction = False
            isSuccess = self.hDb.commit()
        except:
            self.last_error = traceback.format_exc()
            isSuccess = False

        return isSuccess

    def rollback( self ):
        isSuccess = False

        try:
            self.isInTransaction = False
            isSuccess = self.hDb.rollback()
        except:
            self.last_error = traceback.format_exc()
            isSuccess = False

        return isSuccess

    def startTransaction( self ):
        self.isInTransaction = True








class CassDb( object ):

    CL_Ingest = ConsistencyLevel.ONE
    CL_Reliable = ConsistencyLevel.QUORUM
    CL_Default = ConsistencyLevel.TWO

    def __init__( self, url, dbname, version = '3.3.1', quorum = False, consistencyOne = False, backoffConsistency = False ):

        self.isShutdown = False
        self.url = url
        self.dbname = dbname
        self.version = version
        self.consistency = self.CL_Default
        self.cluster = Cluster( url, cql_version = version, control_connection_timeout = 30.0 )
        self.cur = self.cluster.connect( dbname )
        self.cur.default_timeout = 30.0
        self.backoffConsistency = backoffConsistency

        if quorum:
            self.consistency = self.CL_Reliable
        elif consistencyOne:
            self.consistency = self.CL_Ingest

    def __del__( self ):
        self.shutdown()

    def execute( self, query, params = {} ):
        res = None

        thisCur = self.cur

        def queryWith( stmt ):
            nRetry = 0
            isSuccess = False
            while ( not isSuccess ) and ( nRetry < 2 ):
                res = None
                try:
                    res = thisCur.execute( stmt, params )
                    isSuccess = True
                except Exception as e:
                    desc = traceback.format_exc()
                    if 'OperationTimedOut' not in desc:
                        raise e
                    else:
                        gevent.sleep( 5 )
                        syslog.syslog( syslog.LOG_USER, 'TIMEDOUT RETRYING' )
                        nRetry += 1
            return res

        if type( query ) is str or type( query ) is unicode:
            q = SimpleStatement( query, consistency_level = self.consistency )
        else:
            q = query

        try:
            res = queryWith( q )
        except:
            if self.backoffConsistency:
                if ( type( query ) is str or type( query ) is unicode ):
                    q = SimpleStatement( query, consistency_level = self.CL_Ingest )
                else:
                    q = query
                #syslog.syslog( syslog.LOG_USER, 'CONSISTENCY FAILED, BACKOFF' )
                res = queryWith( q )
            else:
                raise

        return res

    def getOne( self, query, params = {} ):
        res = self.execute( query, params )
        if res is not None:
            if 0 == len( res ):
                res = None
            else:
                res = res[ 0 ]
        return res

    def prepare( self, query ):
        return self.cur.prepare( query )

    def execute_async( self, query, params = {} ):
        if type( query ) is str or type( query ) is unicode:
            query = SimpleStatement( query, consistency_level = self.consistency )
        return self.cur.execute_async( query, params )

    def shutdown( self ):
        if not self.isShutdown:
            self.isShutdown = True
            self.cur.shutdown()
            self.cluster.shutdown()
            time.sleep( 0.5 )

class CassPool( object ):

    def __init__( self, db, maxConcurrent = 1, error_log_func = None, rate_limit_per_sec = None, blockOnQueueSize = None ):
        self.maxConcurrent = maxConcurrent
        self.db = db
        self.error_log = error_log_func
        self.isRunning = False
        self.rate_limit = rate_limit_per_sec
        self.rate = 0
        self.last_rate_time = 0
        self.blockOnQueueSize = blockOnQueueSize
        self.queries = deque()
        self.threads = gevent.pool.Group()
        for _ in range( self.maxConcurrent ):
            self._addHandler()

    @synchronized()
    def rateLimit( self, isAdvisory = False ):
        limit = self.rate_limit
        if limit is not None:
            now = int( time.time() )
            if self.last_rate_time != now:
                self.last_rate_time = now
                self.rate = 0
            self.rate += 1
            if not isAdvisory and limit < self.rate:
                syslog.syslog( syslog.LOG_USER, 'RATE LIMIT' )
                gevent.sleep( 1 )
                self.rate = 0

    def execute_async( self, query, params = None ):
        self.queries.append( ( query, params ) )
        while self.blockOnQueueSize is not None and len( self.queries ) > self.blockOnQueueSize:
            syslog.syslog( syslog.LOG_USER, 'BLOCKING' )
            gevent.sleep( 1 )

    def execute( self, query, params = {} ):
        self.rateLimit( isAdvisory = True )
        return self.db.execute( query, params )

    def _addHandler( self ):
        self.threads.add( gevent.spawn( self._queryThread ) )

    def _queryThread( self ):
        while True:
            while not self.isRunning:
                gevent.sleep( 1 )
            try:
                q = self.queries.popleft()
            except:
                q = None

            if q is not None:
                self.rateLimit()
                try:
                    self.db.execute( *q )
                    #syslog.syslog( syslog.LOG_USER, '+' )
                except:
                    syslog.syslog( syslog.LOG_USER, 'EXCEPTION: %s' % traceback.format_exc() )
            else:
                gevent.sleep( 1 )
                #syslog.syslog( syslog.LOG_USER, 'WAITING SUCCESS' )


    def start( self ):
        self.isRunning = True

    def stop( self):
        self.isRunning = False

    def numQueued( self ):
        return len( self.queries )

    def shutdown( self ):
        self.threads.join( timeout = 10 )
        self.threads.kill( timeout = 10 )
        return self.db.shutdown()

    def prepare( self, query ):
        return self.db.prepare( query )

    def getOne( self, query, params = {} ):
        self.rateLimit( isAdvisory = True )
        return self.db.getOne( query, params )
