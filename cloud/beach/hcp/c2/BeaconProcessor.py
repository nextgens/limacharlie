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
import base64
import M2Crypto
import zlib
import time
import struct
import binascii
import traceback
import hashlib
rpcm = Actor.importLib( '../rpcm', 'rpcm' )
rList = Actor.importLib( '../rpcm', 'rList' )
rSequence = Actor.importLib( '../rpcm', 'rSequence' )
AgentId = Actor.importLib( '../hcp_helpers', 'AgentId' )
HcpDb = Actor.importLib( '../hcp_databases', 'HcpDb' )
ip_to_tuple = Actor.importLib( '../hcp_helpers', 'ip_to_tuple' )
HcpOperations = Actor.importLib( '../hcp_helpers', 'HcpOperations' )
HcpModuleId = Actor.importLib( '../hcp_helpers', 'HcpModuleId' )
PooledResource = Actor.importLib( '../hcp_helpers', 'PooledResource' )

class BeaconProcessor( Actor ):
    def init( self, parameters ):
        self.private_key = M2Crypto.RSA.load_key( parameters[ 'priv_key' ] )
        self.handle( 'beacon', self.processBeacon )
        self.symbols = self.importLib( '../Symbols', 'Symbols' )()
        self.enrollment_key = parameters.get( 'enrollment_token', 'DEFAULT_HCP_ENROLLMENT_TOKEN' )
        self.dbPool = PooledResource( lambda: HcpDb( parameters[ 'state_db' ][ 'url' ],
                                                     parameters[ 'state_db' ][ 'db' ],
                                                     parameters[ 'state_db' ][ 'user' ],
                                                     parameters[ 'state_db' ][ 'password' ],
                                                     isConnectRightAway = True ) )
        self.analytics_intake = self.getActorHandle( 'analytics/intake' )

    def deinit( self ):
        pass

    def processBeacon( self, msg ):
        response = ''

        session = {}
        session[ 'payload' ] = msg.data[ 'payload' ]
        session[ 'public_ip' ] = msg.data[ 'public_ip' ]
        session[ 'tx_id' ] = msg.id

        try:
            self._1_unwrap( session )
            self._2_setup( session )
            self._3_update_state( session )

            if HcpModuleId.HCP == session[ 'moduleid' ]:
                self._4_hcp_process( session )
            elif HcpModuleId.HBS == session[ 'moduleid' ]:
                self._4_hbs_process( session )

            self._5_send_for_analysis( session )

            self._6_wrap( session )

        except:
            # This is where we should log session and the exception that will contain
            # all the information required to debug
            self.log( traceback.format_exc() )
            self.log( session )
            pass
        else:
            response = session[ 'response' ]

        return ( True, { 'resp' : response } )

    def _1_unwrap( self, session ):
        rsa_2048_min_size = 0x100
        aes_256_iv_size = 0x10
        aes_256_key_size = 0x20
        rsa_header_size = 0x100 + 0x10 # rsa_2048_min_size + aes_256_iv_size
        rsa_signature_size = 0x100

        payload = session[ 'payload' ]

        payload = base64.b64decode( payload )

        r = rpcm( isHumanReadable = True, isDebug = self.logCritical )
        r.loadSymbols( self.symbols.lookups )

        if rsa_2048_min_size > len( payload ):
            raise Exception( 'not enough data for valid fast asym encrypted' )

        enc_sym_key = payload[ : rsa_2048_min_size ]
        sym_iv = payload[ rsa_2048_min_size : rsa_2048_min_size + aes_256_iv_size ]
        sym_payload = payload[ rsa_2048_min_size + aes_256_iv_size : ]
        sym_key = self.private_key.private_decrypt( enc_sym_key, M2Crypto.RSA.pkcs1_padding )

        if not sym_key:
            raise Exception( 'no valid symetric key found' )

        sym_key = sym_key[ : aes_256_key_size ]
        aes = M2Crypto.EVP.Cipher( alg = 'aes_256_cbc', key = sym_key, iv = sym_iv, salt = False, key_as_bytes = False, op = 0 ) # 0 == DEC
        decrypted = aes.update( sym_payload )
        decrypted += aes.final()
        del( aes )

        decompressed = zlib.decompress( decrypted )

        r.setBuffer( decompressed )

        headers = r.deserialise()

        if 0 != len( r.dataInBuffer() ):
            tmpMessagesBuffer = r.dataInBuffer()
            messages = r.deserialise( isList = True )
            if messages is None:
                session[ 'raw_messages' ] = tmpMessagesBuffer
                raise Exception( 'error deserializing messages from client' )
        else:
            messages = []

        session[ 'headers' ] = headers
        session[ 'messages' ] = messages

    def _2_setup( self, session ):
        session[ 'moduleid' ] = session[ 'headers' ][ 'hcp.MODULE_ID' ]
        session[ 'agentid' ] = session[ 'headers' ][ 'base.HCP_ID' ]
        session[ 'sym_key' ] = session[ 'headers' ][ 'base.SYM_KEY' ]
        session[ 'sym_iv' ] = session[ 'headers' ][ 'base.SYM_IV' ]
        if 'base.IP_ADDRESS' in session[ 'headers' ]:
            session[ 'internal_ip' ] = session[ 'headers' ][ 'base.IP_ADDRESS' ]
        if 'base.HOST_NAME' in session[ 'headers' ]:
            session[ 'hostname' ] = session[ 'headers' ][ 'base.HOST_NAME' ]

        tmpAgent = AgentId( session[ 'agentid' ] )
        if not tmpAgent.isValid or tmpAgent.isWildcarded():
            raise Exception( 'invalid agent id or agent id with wildcards' )

        enrollment_token = session[ 'headers' ].get( 'hcp.ENROLLMENT_TOKEN', None )
        session[ 'valid_enrollment_token' ] = False

        expected_token = hashlib.md5( '%s/%s' % ( tmpAgent.invariableToString(), self.enrollment_key ) ).digest()
        if expected_token == enrollment_token:
            session[ 'valid_enrollment_token' ] = True
        
        session[ 'agentid' ] = tmpAgent
        
        session[ 'outbound_messages' ] = []
        session[ 'analysis_events' ] = []

    def _3_update_state( self, session ):

        def getRelocRange( db, sessionInfo ):
            newRange = None

            agentid = sessionInfo[ 'agentid' ]

            record = db.queryOne( '''SELECT new_org, new_subnet FROM hcp_agent_reloc WHERE %s''' % agentid.asWhere( isWithConfig = False) )

            if False != record:
                newOrg = record[ 'new_org' ]
                newSubnet = record[ 'new_subnet' ]

                newRange = ( newOrg, newSubnet )

            return newRange

        def getEnrollmentAgentId( db, sessionInfo, override_org = False, override_sub = False ):
            newId = False

            agentid = sessionInfo[ 'agentid' ]

            public_ip = ip_to_tuple( sessionInfo[ 'public_ip' ] )
            if 'internal_ip' in sessionInfo:
                private_ip = ip_to_tuple( sessionInfo[ 'internal_ip' ] )
            else:
                private_ip = [ 0xFF, 0xFF, 0xFF, 0xFF ]
            if 'hostname' in sessionInfo:
                hostname = sessionInfo[ 'hostname' ]
            else:
                hostname = ''

            enrollRecord = False

            if ( override_org == False ) and ( override_sub == False ):

                enrollRecord = db.queryOne( '''SELECT new_org, new_subnet FROM hcp_enrollment_rules WHERE
                                               %s AND
                                               ( ( e_ip_A = %%s OR e_ip_A = 255 ) AND
                                                 ( e_ip_C = %%s OR e_ip_C = 255 ) AND
                                                 ( e_ip_B = %%s OR e_ip_B = 255 ) AND
                                                 ( e_ip_D = %%s OR e_ip_D = 255 ) )
                                               AND
                                               ( ( i_ip_A = %%s OR i_ip_A = 255 ) AND
                                                 ( i_ip_B = %%s OR i_ip_B = 255 ) AND
                                                 ( i_ip_C = %%s OR i_ip_C = 255 ) AND
                                                 ( i_ip_D = %%s OR i_ip_D = 255 ) )
                                               AND
                                               ( hostname = %%s OR hostname = '' OR hostname IS NULL )''' % agentid.asWhere(),
                                                                                                            ( public_ip[ 0 ],
                                                                                                              public_ip[ 1 ],
                                                                                                              public_ip[ 2 ],
                                                                                                              public_ip[ 3 ],
                                                                                                              private_ip[ 0 ],
                                                                                                              private_ip[ 1 ],
                                                                                                              private_ip[ 2 ],
                                                                                                              private_ip[ 3 ],
                                                                                                              hostname ) )
            else:
                # We are using the override if it's available. It is used to specify relocations.
                enrollRecord = {}
                enrollRecord[ 'new_org' ] = override_org
                enrollRecord[ 'new_subnet' ] = override_sub

            if False != enrollRecord:
                newOrg = enrollRecord[ 'new_org' ]
                newSubnet = enrollRecord[ 'new_subnet' ]

                db.startTransaction()

                nextIdRecord = db.queryOne( '''SELECT MAX( uniqueid ) AS uniqueid FROM hcp_agentinfo WHERE org = %s AND subnet = %s FOR UPDATE''',
                                                 ( newOrg, newSubnet ) )

                if False != nextIdRecord:
                    nextUnique = 1
                    if None != nextIdRecord[ 'uniqueid' ]:
                        nextUnique = nextIdRecord[ 'uniqueid' ] + 1
                    newId = AgentId( [ newOrg, newSubnet, nextUnique, agentid.platform, agentid.config ] )

                    if db.execute( '''INSERT INTO hcp_agentinfo ( org,
                                                                  subnet,
                                                                  uniqueid,
                                                                  platform,
                                                                  config,
                                                                  last_e_ip_A,
                                                                  last_e_ip_B,
                                                                  last_e_ip_C,
                                                                  last_e_ip_D,
                                                                  last_i_ip_A,
                                                                  last_i_ip_B,
                                                                  last_i_ip_C,
                                                                  last_i_ip_D,
                                                                  enrollment,
                                                                  lastseen )
                                                                  VALUES ( %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, NOW(), NOW() )''',
                                        ( newId.org, newId.subnet, newId.unique, newId.platform, newId.config,
                                          public_ip[ 0 ], public_ip[ 1 ], public_ip[ 2 ], public_ip[ 3 ],
                                          private_ip[ 0 ], private_ip[ 1 ], private_ip[ 2 ], private_ip[ 3 ] ) ):

                        db.commit()
                    else:
                        newId = False
                        db.rollback()
                else:
                    self.log( 'could not fetch new uniqueid for enrollment of %s' % agentid )
                    db.rollback()
            else:
                self.log( 'could not find enrollment rule for %s' % agentid )

            return newId
        
        def registerAgent( db, sessionInfo ):
            isSuccess = False

            public_ip = ip_to_tuple( sessionInfo[ 'public_ip' ] )
            if 'internal_ip' in sessionInfo:
                private_ip = ip_to_tuple( sessionInfo[ 'internal_ip' ] )
            else:
                private_ip = [ 0xFF, 0xFF, 0xFF, 0xFF ]

            newId = sessionInfo[ 'agentid' ]

            if db.execute( '''INSERT INTO hcp_agentinfo ( org,
                                                          subnet,
                                                          uniqueid,
                                                          platform,
                                                          config,
                                                          last_e_ip_A,
                                                          last_e_ip_B,
                                                          last_e_ip_C,
                                                          last_e_ip_D,
                                                          last_i_ip_A,
                                                          last_i_ip_B,
                                                          last_i_ip_C,
                                                          last_i_ip_D,
                                                          enrollment,
                                                          lastseen )
                                                          VALUES ( %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, 0, NOW() )''',
                             ( newId.org, newId.subnet, newId.unique, newId.platform, newId.config,
                               public_ip[ 0 ], public_ip[ 1 ], public_ip[ 2 ], public_ip[ 3 ],
                               private_ip[ 0 ], private_ip[ 1 ], private_ip[ 2 ], private_ip[ 3 ] ) ):
                isSuccess = True

            return isSuccess

        def isNeedsEnrollment( sessionInfo ):
            isNeeded = False
            if 0 == sessionInfo[ 'agentid' ].unique and HcpModuleId.HCP == sessionInfo[ 'moduleid' ]:
                isNeeded = True
            return isNeeded

        def isKnownAgent( db, sessionInfo ):
            isKnown = False
            rec = db.query( 'SELECT lastseen FROM hcp_agentinfo WHERE %s' % sessionInfo[ 'agentid' ].asWhere( isWithConfig = False ) )
            if rec is False:
                self.logCritical( db.last_error )
            elif 0 != len( rec ):
                isKnown = True
            return isKnown


        def touchAgentState( db, sessionInfo ):
            isSuccess = False

            public_ip = ip_to_tuple( sessionInfo[ 'public_ip' ] )
            if 'internal_ip' in sessionInfo:
                private_ip = ip_to_tuple( sessionInfo[ 'internal_ip' ] )
            else:
                private_ip = [ 0xFF, 0xFF, 0xFF, 0xFF ]

            if 'hostname' in sessionInfo:
                hostname = sessionInfo[ 'hostname' ]
            else:
                hostname = ''

            if db.execute( '''UPDATE hcp_agentinfo SET config = %%s,
                                                       last_e_ip_A = %%s, last_e_ip_B = %%s, last_e_ip_C = %%s, last_e_ip_D = %%s,
                                                       last_i_ip_A = %%s, last_i_ip_B = %%s, last_i_ip_C = %%s, last_i_ip_D = %%s,
                                                       lastseen = NOW(), last_hostname = %%s WHERE %s''' % sessionInfo[ 'agentid' ].asWhere( isWithConfig = False ),
                                ( sessionInfo[ 'agentid' ].config,
                                  public_ip[ 0 ], public_ip[ 1 ], public_ip[ 2 ], public_ip[ 3 ],
                                  private_ip[ 0 ], private_ip[ 1 ], private_ip[ 2 ], private_ip[ 3 ], hostname ) ):
                isSuccess = True

            return isSuccess

        db = self.dbPool.acquire()

        if isNeedsEnrollment( session ):
            self.log( 'agent %s requires enrollment' % session[ 'agentid' ] )
            
            newId = getEnrollmentAgentId( db, session )
            
            if newId is False:
                raise Exception( 'error during enrollment' )
            
            self.log( 'agent now enrolled as %s' % newId )
            session[ 'agentid' ] = newId

            newEnrollmentToken = hashlib.md5( '%s/%s' % ( newId.invariableToString(), self.enrollment_key ) ).digest()
            
            session[ 'outbound_messages' ].append( rSequence().addInt8( self.symbols.base.OPERATION, HcpOperations.SET_HCP_ID )
                                                              .addSequence( self.symbols.base.HCP_ID, newId.toJson() )
                                                              .addBuffer( self.symbols.hcp.ENROLLMENT_TOKEN, newEnrollmentToken ) )
            
        elif not isKnownAgent( db, session ) and session[ 'valid_enrollment_token' ] is True:
            # We must register this already-existing agent, might belong to another C2
            self.log( 'agent %s requires registration' % session[ 'agentid' ] )
            if not registerAgent( db, session ):
                raise Exception( 'error registering agent' )
            
        elif session[ 'valid_enrollment_token' ] is True:
            if not touchAgentState( db, session ):
                raise Exception( 'error updating agent' )
            
            relocRange = getRelocRange( db, session )
            if relocRange is not None:
                self.log( 'agent %s requires relocation enrollment' % session[ 'agentid' ] )
            
                newId = getEnrollmentAgentId( db, session, relocRange[ 0 ], relocRange[ 1 ] )
                
                if newId is False:
                    raise Exception( 'error during relocation enrollment' )
                
                self.log( 'agent now reloc-enrolled as %s' % newId )
                session[ 'agentid' ] = newId

                newEnrollmentToken = hashlib.md5( '%s/%s' % ( newId.invariableToString(), self.enrollment_key ) ).digest()

                session[ 'outbound_messages' ].append( rSequence().addInt8( self.symbols.base.OPERATION, HcpOperations.SET_HCP_ID )
                                                                  .addSequence( self.symbols.base.HCP_ID, newId.toJson() )
                                                                  .addBuffer( self.symbols.hcp.ENROLLMENT_TOKEN, newEnrollmentToken ) )
        else:
            raise Exception( 'invalid enrollment token, impersonation attempt?' )
        self.dbPool.release( db )

    def _4_hcp_process( self, session ):
        def getModulesToLoad( db, agentid ):
            modules = {}

            modRecords = db.query( '''SELECT moduleid, hash FROM hcp_module_tasking WHERE %s''' % agentid.asWhere() )
            if modRecords:
                for rec in modRecords:
                    modules[ rec[ 'moduleid' ] ] = rec[ 'hash' ]

            return modules

        def getModuleContent( db, moduleId, hashVal ):
            content = None
            sig = None

            rec = db.queryOne( 'SELECT bin, signature FROM hcp_modules WHERE moduleid = %s AND hash = %s', ( moduleId, hashVal ) )

            if rec:
                content = rec[ 'bin' ]
                sig = rec[ 'signature' ]

            return ( content, sig )

        def updateAgentStatus( db, agentid, modulesLoaded, memUsed ):
            if 0 != len( modulesLoaded.values() ):
                db.execute( '''UPDATE hcp_agentinfo SET lastmodule = %%s, memused = %%s WHERE %s''' % agentid.asWhere(), ( modulesLoaded.values()[ 0 ], memUsed ) )

        db = self.dbPool.acquire()

        #===========================================================
        #   Forward To Analytics Cloud
        #===========================================================
        i = 0
        for message in session[ 'messages' ]:
            # We normalize the HCP beacon as we send it to analysis
            session[ 'analysis_events' ].append( ( { 'agentid' : session[ 'agentid' ],
                                                     'moduleid' : session[ 'moduleid' ],
                                                     'event_type' : 'HCP_BEACON',
                                                     'event_id' : hashlib.sha256( '%s-%d' % ( session[ 'tx_id' ], i ) ).hexdigest() },
                                                   { 'HCP_BEACON' : message } ) )
            i += 1

        #===========================================================
        #   Sync Global Time
        #===========================================================
        session[ 'outbound_messages' ].append( rSequence().addInt8( self.symbols.base.OPERATION,
                                                                    HcpOperations.SET_GLOBAL_TIME )
                                                          .addTimestamp( self.symbols.base.TIMESTAMP,
                                                                         int( time.time() ) ) )

        #===========================================================
        #   Set Next Beacon
        #===========================================================
        nextBeacon = Scheduler( db ).getNextAvailableTimeSlice( 'hcp', session[ 'agentid' ] )
        if nextBeacon is not None:
            session[ 'outbound_messages' ].append( rSequence().addInt8( self.symbols.base.OPERATION,
                                                                        HcpOperations.SET_NEXT_BEACON )
                                                              .addTimedelta( self.symbols.base.TIMEDELTA,
                                                                             nextBeacon ) )
        else:
            self.logCritical( 'error getting new hcp beacon time for agent' )

        #===========================================================
        #   Module to Load
        #===========================================================
        # Get the messages that should be loaded
        modulesToLoad = getModulesToLoad( db, session[ 'agentid' ] )
        self.log( 'agent should have %d modules loaded' % len( modulesToLoad ) )

        # Get the messages that are currently loaded
        modulesLoaded = {}
        memUsed = 0
        for message in session[ 'messages' ]:
            for mod in message[ 'hcp.MODULES' ]:
                modulesLoaded[ mod[ 'hcp.MODULE_ID' ] ] = binascii.b2a_hex( mod[ 'base.HASH' ] )
            if 'hcp.MEMORY_USAGE' in message:
                memUsed = message[ 'hcp.MEMORY_USAGE' ]

        self.log( 'agent has the following modules loaded: %s' % str( modulesLoaded ) )

        # Find modules to Unload
        toUnload = [ x for x in modulesLoaded if ( x not in modulesToLoad or modulesLoaded[ x ] != modulesToLoad[ x ] ) ]

        # Find modules to Load
        toLoad = [ x for x in modulesToLoad if ( x not in modulesLoaded  or modulesToLoad[ x ] != modulesLoaded[ x ] ) ]

        # Send the relevant commands
        for mod in toUnload:
            session[ 'outbound_messages' ].append( rSequence().addInt8( self.symbols.base.OPERATION,
                                                                        HcpOperations.UNLOAD_MODULE )
                                                              .addInt8( self.symbols.hcp.MODULE_ID,
                                                                        int( mod ) ) )
            self.log( 'issued a UNLOAD_MODULE for module id %s' % mod )

        for mod in toLoad:
            ( modContent, modSig ) = getModuleContent( db, mod, modulesToLoad[ mod ] )
            if None != modContent and None != modSig:
                session[ 'outbound_messages' ].append( rSequence().addInt8( self.symbols.base.OPERATION,
                                                                            HcpOperations.LOAD_MODULE )
                                                                  .addInt8( self.symbols.hcp.MODULE_ID,
                                                                            int( mod ) )
                                                                  .addBuffer( self.symbols.base.BINARY,
                                                                              modContent )
                                                                  .addBuffer( self.symbols.base.SIGNATURE,
                                                                              modSig ) )
                self.log( 'issued a LOAD_MODULE for module id %s and hash %s' % ( mod, modulesToLoad[ mod ] ) )
            else:
                self.logCritical( 'looked for HCP module id %s and hash %s but could not find them!' % ( mod, modulesToLoad[ mod ] ) )

        updateAgentStatus( db, session[ 'agentid' ], modulesLoaded, memUsed )

        self.dbPool.release( db )


    def _4_hbs_process( self, session ):
        def getConfiguration( r, db, agentid ):
            config = None
            configHash = None

            modRecords = db.queryOne( '''SELECT modconfigs, confighash FROM hbs_configurations WHERE %s''' % agentid.asWhere() )
            if modRecords:
                try:
                    configHash = modRecords[ 'confighash' ]
                    r.setBuffer( modRecords[ 'modconfigs' ] )
                    config = r.deserialise( isList = True )
                except:
                    self.logCritical( 'error deserializing configuration for agent %s: %s' % ( agentid, traceback.format_exc() ) )
                    config = None
                    configHash = None
            else:
                self.log( 'no hbs configuration profile is available for the agent')

            return config, configHash

        def getTasks( r, db, agentid ):
            tasks = []
            ids = []

            db.startTransaction()

            records = db.query( '''SELECT taskid, task FROM hbs_agent_tasks WHERE %s''' % agentid.asWhere( isWithConfig = False ) )
            if records != False:
                for record in records:
                    try:
                        r.setBuffer( record[ 'task' ] )
                        tasks.append( r.deserialise() )
                    except:
                        self.logCritical( "error deserialising tasking for %s" % agentid )
                    ids.append( str( int( record[ 'taskid' ] ) ) )

                self.log( 'found %d tasks for agent %s' % ( len( tasks ), agentid ) )

                if 0 != len( ids ):
                    if not db.execute( '''DELETE FROM hbs_agent_tasks WHERE taskid IN ( %s )''' % ','.join( ids ) ):
                        self.logCritical( "error deleting tasking for agent %s in database" % agentid )
            else:
                self.logCritical( "error querying databse for tasking: %s" % db.last_error )

            db.commit()

            return tasks

        r = rpcm( isDebug = self.logCritical, isHumanReadable = False, isDetailedDeserialize = True )
        r.loadSymbols( self.symbols.lookups )

        db = self.dbPool.acquire()

        #===========================================================
        #   Forward To Analytics Cloud
        #===========================================================
        i = 0
        for message in session[ 'messages' ]:
            routing = { 'agentid' : session[ 'agentid' ],
                        'moduleid' : session[ 'moduleid' ],
                        'event_type' : message.keys()[ 0 ],
                        'event_id' : hashlib.sha256( '%s-%d' % ( session[ 'tx_id' ], i ) ).hexdigest() }

            # While we traverse all messages we keep a lookout for a message with a single
            # hash in it. It's a special state message sent by HBS to tell us the current
            # config / profile active. Using this we can decide if we need to send it again.
            # We ignore this message and don't forward it to the analytic cloud since it's useless.
            if 1 == len( message ):
                tag, val = message.items()[ 0 ]
                if tag == 'base.HASH':
                    sensorConfigHash = val.encode( 'hex' )
                    continue


            # If an InvestigationId is present, we will expose it to Routing
            if type( message.values()[ 0 ] ) is dict and 'hbs.INVESTIGATION_ID' in message.values()[ 0 ]:
                routing[ 'investigation_id' ] = message.values()[ 0 ][ 'hbs.INVESTIGATION_ID' ]

            session[ 'analysis_events' ].append( ( routing, message ) )
            i += 1

        #===========================================================
        #   Set Next Beacon
        #===========================================================
        nextBeacon = Scheduler( db ).getNextAvailableTimeSlice( 'hbs', session[ 'agentid' ] )
        if nextBeacon is not None and type( nextBeacon ) is not str:
            session[ 'outbound_messages' ].append( rSequence().addTimedelta( self.symbols.base.TIMEDELTA,
                                                                             nextBeacon ) )
        else:
            self.logCritical( 'error getting new hbs beacon time for agent: %s' % ( nextBeacon, ) )


        #===========================================================
        #   Effective Configuration
        #===========================================================
        config, configHash = getConfiguration( r, db, session[ 'agentid' ] )

        self.log( "sensor has profile: %s" % sensorConfigHash )

        if config and configHash != sensorConfigHash:
            session[ 'outbound_messages' ].append( rSequence().addList( self.symbols.hbs.CONFIGURATIONS, config )
                                                              .addBuffer( self.symbols.base.HASH,
                                                                          configHash.decode( 'hex' ) ) )
            self.log( "sending new version of profile to sensor: %s" % configHash )


        #===========================================================
        #   Cloud Tasking
        #===========================================================
        tasks = getTasks( r, db, session[ 'agentid' ] )
        if 0 != len( tasks ):
            taskList = rList()
            for task in tasks:
                taskList.addSequence( self.symbols.hbs.CLOUD_NOTIFICATION, task )
            session[ 'outbound_messages' ].append( rSequence().addList( self.symbols.hbs.CLOUD_NOTIFICATIONS,
                                                                        taskList ) )

        #===========================================================
        #   Debug Mode
        #===========================================================
        # This is a quick placeholder for eventual logic to enable debugMode
        # on specific sensors.
        debugMode = 0
        session[ 'outbound_messages' ].append( rSequence().addInt8( self.symbols.base.DEBUG_MODE,
                                                                    debugMode ) )

        self.dbPool.release( db )

    def _5_send_for_analysis( self, session ):
        self.analytics_intake.shoot( 'analyze', session[ 'analysis_events' ] )

    def _6_wrap( self, session ):
        r = rpcm( isDebug = self.logCritical, isHumanReadable = True )
        r.loadSymbols( self.symbols.lookups )

        #===========================================================
        #   Serialize Outbound Messages
        #===========================================================
        msgList = rList()
        for tmpMsg in session[ 'outbound_messages' ]:
            msgList.addSequence( self.symbols.base.MESSAGE, tmpMsg )

        serialMessages = r.serialise( msgList )

        if None == serialMessages:
            raise Exception( 'error serialising messages' )

        #===========================================================
        #   Compress
        #===========================================================
        compressedMessages = struct.pack( '>I', len( serialMessages ) ) + zlib.compress( serialMessages )

        if compressedMessages is None:
            raise Exception( 'error compressing outbound messages' )

        #===========================================================
        #   Encrypt & Sign
        #===========================================================
        aes = M2Crypto.EVP.Cipher( alg = 'aes_256_cbc',
                                   key = session[ 'sym_key' ],
                                   iv = session[ 'sym_iv' ],
                                   salt = False, key_as_bytes = False, op = 1 ) # 1 == ENC
        encryptedMessages = aes.update( compressedMessages )
        encryptedMessages += aes.final()
        del( aes )

        if None == encryptedMessages:
            raise Exception( 'error encrypting outbound messages with session key' )

        signature = self.private_key.private_encrypt( hashlib.sha256( encryptedMessages ).digest(), M2Crypto.RSA.pkcs1_padding )

        if None == signature:
            raise Exception( 'error signing outbound messages' )

        #===========================================================
        #   Wrap it all up
        #===========================================================
        session[ 'response' ] = base64.b64encode( signature + encryptedMessages )

        if session[ 'response' ] is None:
            raise Exception( 'error encoding outbound messages' )


class Scheduler:

    def __init__( self, db ):
        self.db = db

    def getNextAvailableTimeSlice( self, table, agentId, startTime = None, endTime = None ):
        nextSlice = None

        res = self.db.queryOne( 'SELECT slice FROM %s_schedule_agents WHERE %s' % ( table, agentId.asWhere() ) )

        if res:
            s = res[ 'slice' ]

            self.db.execute( 'UPDATE ' + table + ' _schedule_periods WHERE slice = %s SET numscheduled = numscheduled - 1'  )
        else:
            self.db.execute( 'INSERT INTO ' + table + '_schedule_agents SET ( org, subnet, uniqueid, platform, slice ) VALUES ( %s, %s, %s, %s, 0 )', ( agentId.org, agentId.subnet, agentId.unique, agentId.platform, agentId.config ) )

        whereStr = ''
        whereLims = ()

        if startTime is not None and endTime is not None:
            whereStr = 'WHERE slice >= %s AND slice >= %s'
            whereLims = ( startTime, endTime )

        res = self.db.queryOne( 'SELECT slice FROM %s_schedule_periods %s ORDER BY numscheduled ASC, RAND() LIMIT 1' % ( table, whereStr ), whereLims )

        if res:
            newSlice = res[ 'slice' ]

            self.db.execute( 'UPDATE ' + table + '_schedule_periods WHERE slice = %s SET numscheduled = numscheduled + 1', ( newSlice, ) )
            self.db.execute( ( 'UPDATE %s_schedule_agents WHERE %s' % ( table, agentId.asWhere() ) ) + ' SET slice = %s', ( newSlice, ) )

            res = self.db.queryOne( 'SELECT COUNT( slice ) AS period FROM %s_schedule_periods' % table )
            if res:
                period = res[ 'period' ]
                curSlice = int( time.time() ) % period

                if curSlice > newSlice:
                    nextSlice = ( period - curSlice ) + newSlice
                else:
                    nextSlice = newSlice - curSlice
            else:
                nextSlice = None

        return nextSlice