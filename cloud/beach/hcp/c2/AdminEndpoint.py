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
import traceback
import hashlib
import time
rpcm = Actor.importLib( '../rpcm', 'rpcm' )
rList = Actor.importLib( '../rpcm', 'rList' )
rSequence = Actor.importLib( '../rpcm', 'rSequence' )
AgentId = Actor.importLib( '../hcp_helpers', 'AgentId' )
HbsCollectorId = Actor.importLib( '../hcp_helpers', 'HbsCollectorId' )
HcpDb = Actor.importLib( '../hcp_databases', 'HcpDb' )
ip_to_tuple = Actor.importLib( '../hcp_helpers', 'ip_to_tuple' )
HcpOperations = Actor.importLib( '../hcp_helpers', 'HcpOperations' )
HcpModuleId = Actor.importLib( '../hcp_helpers', 'HcpModuleId' )
PooledResource = Actor.importLib( '../hcp_helpers', 'PooledResource' )

def audited( f ):
    def wrapped( self, *args, **kwargs ):
        self.auditor.shoot( 'audit', { 'data' : args[ 0 ].data, 'cmd' : args[ 0 ].req } )
        r = f( self, *args, **kwargs )
        return r
    return wrapped

class AdminEndpoint( Actor ):
    def init( self, parameters ):
        self.symbols = self.importLib( '../Symbols', 'Symbols' )()
        self.dbPool = PooledResource( lambda: HcpDb( parameters[ 'state_db' ][ 'url' ],
                                                     parameters[ 'state_db' ][ 'db' ],
                                                     parameters[ 'state_db' ][ 'user' ],
                                                     parameters[ 'state_db' ][ 'password' ],
                                                     isConnectRightAway = True ) )
        self.handle( 'ping', self.ping )
        self.handle( 'hcp.get_agent_states', self.cmd_hcp_getAgentStates )
        self.handle( 'hcp.get_period', self.cmd_hcp_getPeriod )
        self.handle( 'hcp.set_period', self.cmd_hcp_setPeriod )
        self.handle( 'hcp.get_enrollment_rules', self.cmd_hcp_getEnrollmentRules )
        self.handle( 'hcp.get_agent_states', self.cmd_hcp_getAgentStates )
        self.handle( 'hcp.add_enrollment_rule', self.cmd_hcp_addEnrollmentRule )
        self.handle( 'hcp.del_enrollment_rule', self.cmd_hcp_delEnrollmentRule )
        self.handle( 'hcp.get_taskings', self.cmd_hcp_getTaskings )
        self.handle( 'hcp.add_tasking', self.cmd_hcp_addTasking )
        self.handle( 'hcp.remove_tasking', self.cmd_hcp_delTasking )
        self.handle( 'hcp.get_modules', self.cmd_hcp_getModules )
        self.handle( 'hcp.add_module', self.cmd_hcp_addModule )
        self.handle( 'hcp.remove_module', self.cmd_hcp_delModule )
        self.handle( 'hcp.reloc_agent', self.cmd_hcp_relocAgent )
        self.handle( 'hcp.get_relocations', self.cmd_hcp_getRelocs )
        self.handle( 'hbs.get_period', self.cmd_hbs_getPeriod )
        self.handle( 'hbs.set_period', self.cmd_hbs_setPeriod )
        self.handle( 'hbs.set_profile', self.cmd_hbs_addProfile )
        self.handle( 'hbs.get_profiles', self.cmd_hbs_getProfiles )
        self.handle( 'hbs.del_profile', self.cmd_hbs_delProfile )
        self.handle( 'hbs.task_agent', self.cmd_hbs_taskAgent )

        self.auditor = self.getActorHandle( 'c2/auditing', timeout = 5, nRetries = 3 )

    def deinit( self ):
        pass

    def ping( self, msg ):
        return ( True, { 'pong' : time.time() } )

    @audited
    def cmd_hcp_getAgentStates( self, msg ):
        with self.dbPool.anInstance() as db:
            data = {}
            request = msg.data

            where = ''
            v = []
            if 'agent_id' in request:
                aid = AgentId( request[ 'agent_id' ] )
                where = 'WHERE %s' % aid.asWhere( isWithConfig = False )
            elif 'hostname' in request:
                where ='WHERE last_hostname = %s'
                v.append( request[ 'hostname' ] )

            results = db.query( 'SELECT * FROM hcp_agentinfo %s' % where, v )

            if results is not False:
                data[ 'agents' ] = {}
                for result in results:
                    a = {}
                    a[ 'agent_id' ] = str( AgentId( result ) )
                    a[ 'last_external_ip' ] = '%s.%s.%s.%s' % ( result[ 'last_e_ip_A' ],
                                                                result[ 'last_e_ip_B' ],
                                                                result[ 'last_e_ip_C' ],
                                                                result[ 'last_e_ip_D' ] )
                    a[ 'last_internal_ip' ] = '%s.%s.%s.%s' % ( result[ 'last_i_ip_A' ],
                                                                result[ 'last_i_ip_B' ],
                                                                result[ 'last_i_ip_C' ],
                                                                result[ 'last_i_ip_D' ] )
                    a[ 'last_hostname' ] = result[ 'last_hostname' ]
                    a[ 'enrollment_date' ] = str( result[ 'enrollment' ] )
                    a[ 'date_last_seen' ] = str( result[ 'lastseen' ] )
                    a[ 'last_module' ] = result[ 'lastmodule' ]
                    a[ 'mem_used' ] = result[ 'memused' ]
                    data[ 'agents' ][ a[ 'agent_id' ] ] = a

                response = ( True, data )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_getPeriod( self, msg ):
        with self.dbPool.anInstance() as db:
            result = db.queryOne( 'SELECT COUNT( slice ) AS n FROM hcp_schedule_periods' )

            if result is not False:
                response = ( True, { 'period' : result[ 'n' ] } )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_setPeriod( self, msg ):
        with self.dbPool.anInstance() as db:
            response = ( True, )
            request = msg.data
            result = db.queryOne( 'SELECT COUNT( slice ) AS n FROM hcp_schedule_periods' )

            if result is not False:
                period = result[ 'n' ]

                if period > request[ 'period' ]:
                    if not db.execute( 'DELETE FROM hcp_schedule_periods WHERE slice > %s', request[ 'period' ] ):
                        response = ( False, db.last_error )
                elif period != request[ 'period' ]:
                    if not db.execute( 'INSERT INTO hcp_schedule_periods ( slice, numscheduled ) VALUES %s' % ', '.join( [ '( %d, 0 )' % x for x in range( period, request[ 'period' ] ) ] ) ):
                        response = ( False, db.last_error )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_getEnrollmentRules( self, msg ):
        with self.dbPool.anInstance() as db:
            results = db.query( 'SELECT * FROM hcp_enrollment_rules' )

            if results is not False:
                rules = []

                for record in results:
                    rules.append( { 'mask' : AgentId( record ),
                                    'external_ip' : '%d.%d.%d.%d' % ( record[ 'e_ip_A' ], record[ 'e_ip_B' ], record[ 'e_ip_C' ], record[ 'e_ip_D' ] ),
                                    'internal_ip' : '%d.%d.%d.%d' % ( record[ 'i_ip_A' ], record[ 'i_ip_B' ], record[ 'i_ip_C' ], record[ 'i_ip_D' ] ),
                                    'hostname' : record[ 'hostname' ],
                                    'new_org' : hex( record[ 'new_org' ] ),
                                    'new_subnet' : hex( record[ 'new_subnet' ] ),
                                    'hostname' : record[ 'hostname' ] } )

                response = ( True, { 'rules' : rules } )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_addEnrollmentRule( self, msg ):
        with self.dbPool.anInstance() as db:
            request = msg.data
            response = ( True, )

            mask = AgentId( request[ 'mask' ] )
            e_ip = ip_to_tuple( request[ 'external_ip' ] )
            i_ip = ip_to_tuple( request[ 'internal_ip' ] )
            new_org = request[ 'new_org' ]
            new_subnet = request[ 'new_subnet' ]
            hostname = request[ 'hostname' ]

            if not db.execute( '''INSERT INTO hcp_enrollment_rules ( org, subnet, uniqueid, platform, config,
                                                                     e_ip_A, e_ip_B, e_ip_C, e_ip_D,
                                                                     i_ip_A, i_ip_B, i_ip_C, i_ip_D,
                                                                     new_org, new_subnet, hostname ) VALUES (
                                                                     %s, %s, %s, %s, %s,
                                                                     %s, %s, %s, %s,
                                                                     %s, %s, %s, %s,
                                                                     %s, %s, %s )''', ( mask.org, mask.subnet, mask.unique, mask.platform, mask.config,
                                                                                        e_ip[ 0 ], e_ip[ 1 ], e_ip[ 2 ], e_ip[ 3 ],
                                                                                        i_ip[ 0 ], i_ip[ 1 ], i_ip[ 2 ], i_ip[ 3 ],
                                                                                        new_org, new_subnet,
                                                                                        hostname ) ):
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_delEnrollmentRule( self, msg ):
        with self.dbPool.anInstance() as db:
            response = ( True, )
            request = msg.data

            mask = AgentId( request[ 'mask' ] )
            e_ip = ip_to_tuple( request[ 'external_ip' ] )
            i_ip = ip_to_tuple( request[ 'internal_ip' ] )
            new_org = request[ 'new_org' ]
            new_subnet = request[ 'new_subnet' ]
            hostname = request[ 'hostname' ]

            if not db.execute( '''DELETE FROM hcp_enrollment_rules WHERE org = %s AND subnet = %s AND uniqueid = %s AND platform = %s AND config = %s AND
                                                                         e_ip_A = %s AND e_ip_B = %s AND e_ip_C = %s AND e_ip_D = %s AND
                                                                         i_ip_A = %s AND i_ip_B = %s AND i_ip_C = %s AND i_ip_D = %s AND
                                                                         new_org = %s AND new_subnet = %s AND hostname = %s''',
                                                                         ( mask.org, mask.subnet, mask.unique, mask.platform, mask.config,
                                                                           e_ip[ 0 ], e_ip[ 1 ], e_ip[ 2 ], e_ip[ 3 ],
                                                                           i_ip[ 0 ], i_ip[ 1 ], i_ip[ 2 ], i_ip[ 3 ],
                                                                           new_org, new_subnet, hostname ) ):
                response = ( False, db.last_error )


        return response

    @audited
    def cmd_hcp_getTaskings( self, msg ):
        with self.dbPool.anInstance() as db:
            results = db.query( 'SELECT * FROM hcp_module_tasking' )

            if results is not False:
                data = {}
                data[ 'taskings' ] = []

                for record in results:
                    data[ 'taskings' ].append( { 'mask' : AgentId( record ),
                                                 'module_id' : record[ 'moduleid' ],
                                                 'hash' : record[ 'hash' ] } )

                response = ( True, data )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_addTasking( self, msg ):
        with self.dbPool.anInstance() as db:
            request = msg.data
            response = ( True, )
            mask = AgentId( request[ 'mask' ] )
            moduleid = int( request[ 'module_id' ] )
            h = str( request[ 'hash' ] )

            if not db.execute( '''INSERT INTO hcp_module_tasking ( org, subnet, uniqueid, platform, config,
                                                                   moduleid, hash ) VALUES ( %s, %s, %s, %s, %s, %s, %s )''',
                                                                   ( mask.org, mask.subnet, mask.unique, mask.platform, mask.config,
                                                                     moduleid, h ) ):
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_delTasking( self, msg ):
        with self.dbPool.anInstance() as db:
            request = msg.data
            response = ( True, )
            mask = AgentId( request[ 'mask' ] )
            moduleid = int( request[ 'module_id' ] )
            h = str( request[ 'hash' ] )

            if not db.execute( '''DELETE FROM hcp_module_tasking WHERE org = %s AND subnet = %s AND uniqueid = %s AND platform = %s AND config = %s AND
                                                                       moduleid = %s AND hash = %s''',
                                                                       ( mask.org, mask.subnet, mask.unique, mask.platform, mask.config,
                                                                         moduleid, h ) ):
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_getModules( self, msg ):
        with self.dbPool.anInstance() as db:
            results = db.query( 'SELECT moduleid, hash, description FROM hcp_modules' )

            if results is not False:
                data = {}
                data[ 'modules' ] = []

                for record in results:
                    data[ 'modules' ].append( { 'module_id' : record[ 'moduleid' ],
                                                'hash' : record[ 'hash' ],
                                                'description' : record[ 'description' ] } )
                response = ( True, data )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_addModule( self, msg ):
        with self.dbPool.anInstance() as db:
            request = msg.data
            moduleid = int( request[ 'module_id' ] )
            h = str( request[ 'hash' ] )
            b = request[ 'bin' ]
            sig = request[ 'signature' ]
            description = ''
            if 'description' in request:
                description = request[ 'description' ]

            if db.execute( '''INSERT INTO hcp_modules ( moduleid, hash, bin, signature, description ) VALUES ( %s, %s, %s, %s, %s )''',
                                                      ( moduleid, h, b, sig, description ) ):
                data = {}
                data[ 'hash' ] = h
                data[ 'module_id' ] = moduleid
                data[ 'description' ] = description
                response = ( True, data )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_delModule( self, msg ):
        with self.dbPool.anInstance() as db:
            request = msg.data
            moduleid = int( request[ 'module_id' ] )
            h = str( request[ 'hash' ] )

            if db.execute( '''DELETE FROM hcp_modules WHERE moduleid = %s AND hash = %s''',
                              ( moduleid, h ) ):
                response = ( True, )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hcp_relocAgent( self, msg ):
        with self.dbPool.anInstance() as db:
            request = msg.data
            fromAgent = AgentId( request[ 'agentid' ] )
            newOrg = request[ 'new_org' ]
            newSub = request[ 'new_subnet' ]

            if not db.execute( 'INSERT INTO hcp_agent_reloc ( org, subnet, uniqueid, platform, new_org, new_subnet ) VALUES ( %s, %s, %s, %s, %s, %s )',
                                ( fromAgent.org, fromAgent.subnet, fromAgent.unique, fromAgent.platform, newOrg, newSub ) ):
                response = ( False, db.last_error )
            else:
                response = ( True, )

        return response

    @audited
    def cmd_hcp_getRelocs( self, msg ):
        with self.dbPool.anInstance() as db:
            results = db.query( 'SELECT * FROM hcp_agent_reloc' )

            if results is not False:
                data = {}
                data[ 'relocations' ] = {}
                for reloc in results:
                    data[ 'relocations' ][ str( AgentId( reloc ) ) ] = { 'agentid' : AgentId( reloc ), 'new_org' : reloc[ 'new_org' ], 'new_subnet' : reloc[ 'new_subnet' ] }

                response = ( True, data )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hbs_getPeriod( self, msg ):
        with self.dbPool.anInstance() as db:
            result = db.queryOne( 'SELECT COUNT( slice ) AS n FROM hbs_schedule_periods' )

            if result is not False:
                response = ( True, { 'period' : result[ 'n' ] } )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hbs_setPeriod( self, msg ):
        with self.dbPool.anInstance() as db:
            response = ( True, )
            request = msg.data
            result = db.queryOne( 'SELECT COUNT( slice ) AS n FROM hbs_schedule_periods' )

            if result is not False:
                period = result[ 'n' ]

                if period > request[ 'period' ]:
                    if not db.execute( 'DELETE FROM hbs_schedule_periods WHERE slice > %s', request[ 'period' ] ):
                        response = ( False, db.last_error )
                elif period != request[ 'period' ]:
                    if not db.execute( 'INSERT INTO hbs_schedule_periods ( slice, numscheduled ) VALUES %s' % ', '.join( [ '( %d, 0 )' % x for x in range( period, request[ 'period' ] ) ] ) ):
                        response = ( False, db.last_error )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hbs_getProfiles( self, msg ):
        with self.dbPool.anInstance() as db:
            results = db.query( 'SELECT * FROM hbs_configurations' )

            if results is not False:
                data = {}
                data[ 'profiles' ] = []

                for record in results:
                    data[ 'profiles' ].append( { 'mask' : AgentId( record ),
                                                 'original_configs' : record[ 'originalconfigs' ] } )

                response = ( True, data )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hbs_addProfile( self, msg ):
        with self.dbPool.anInstance() as db:
            request = msg.data
            mask = AgentId( request[ 'mask' ] )
            c = request[ 'module_configs' ]
            isValidConfig = False
            profileError = ''
            oc = c
            configHash = None

            if c is not None and '' != c:
                r = rpcm( isDebug = True )
                rpcm_environment = { '_' : self.symbols,
                                     'rList' : rList,
                                     'rSequence' : rSequence,
                                     'HbsCollectorId' : HbsCollectorId }
                try:
                    profile = eval( c.replace( '\n', '' ), rpcm_environment )
                except:
                    profile = None
                    profileError = traceback.format_exc()

                if profile is not None:
                    if type( profile ) is rList:
                        profile = r.serialise( profile )

                        if profile is not None:
                            isValidConfig = True
                            c = profile
                            configHash = hashlib.sha256( profile ).hexdigest()
                        else:
                            profileError = 'config could not be serialised'
                    else:
                        profileError = 'config did not evaluate as an rList: %s' % type( profile )

            else:
                isValidConfig = True

            if isValidConfig:
                response = ( True, )
                if not db.execute( '''INSERT INTO hbs_configurations ( org, subnet, uniqueid, platform, config, modconfigs, originalconfigs, confighash ) VALUES ( %s, %s, %s, %s, %s, %s, %s, %s )''',
                                                                     ( mask.org, mask.subnet, mask.unique, mask.platform, mask.config, c, oc, configHash ) ):
                    response = ( False, db.last_error )
            else:
                response = ( False, profileError )

        return response

    @audited
    def cmd_hbs_delProfile( self, msg ):
        with self.dbPool.anInstance() as db:
            request = msg.data
            mask = AgentId( request[ 'mask' ] )

            if db.execute( '''DELETE FROM hbs_configurations WHERE org = %s AND subnet = %s AND uniqueid = %s AND platform = %s AND config = %s''',
                                                                 ( mask.org, mask.subnet, mask.unique, mask.platform, mask.config ) ):
                response = ( True, )
            else:
                response = ( False, db.last_error )

        return response

    @audited
    def cmd_hbs_taskAgent( self, msg ):
        with self.dbPool.anInstance() as db:
            request = msg.data
            agent = AgentId( request[ 'agentid' ] )
            task = request[ 'task' ]

            if db.execute( '''INSERT INTO hbs_agent_tasks ( org, subnet, uniqueid, platform, task ) VALUES ( %s, %s, %s, %s, %s )''',
                                                          ( agent.org, agent.subnet, agent.unique, agent.platform, task ) ):
                response = ( True, )
            else:
                response = ( False, db.last_error )

        return response