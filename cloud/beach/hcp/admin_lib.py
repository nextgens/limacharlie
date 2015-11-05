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

from beach.beach_api import Beach
import json
import hashlib
try:
    # When used by the random python script
    # import normally
    from rpcm import rpcm
    from rpcm import rSequence
    from rpcm import rList
    from Symbols import Symbols
    from signing import Signing
    from hcp_helpers import AgentId
except:
    # When in an actor, use the relative import
    from beach.actor import Actor
    rpcm = Actor.importLib( 'rpcm', 'rpcm' )
    rSequence = Actor.importLib( 'rpcm', 'rSequence' )
    rList = Actor.importLib( 'rpcm', 'rList' )
    Symbols = Actor.importLib( 'Symbols', 'Symbols' )
    Signing = Actor.importLib( 'signing', 'Signing' )
    AgentId = Actor.importLib( 'hcp_helpers', 'AgentId' )

#===============================================================================
# Library section to be used by Python code for automation
#===============================================================================
class BEAdmin( object ):
    token = None
    
    def __init__( self, beach_config, token, timeout = 1000 * 10 ):
        self.token = token
        self.beach = Beach( beach_config, realm = 'hcp' )
        self.vHandle = self.beach.getActorHandle( 'c2/admin/1.0',
                                                  ident = 'cli/955f6e63-9119-4ba6-a969-84b38bfbcc05',
                                                  timeout = timeout,
                                                  nRetries = 3 )

    def _query( self, cmd, data = {} ):
        data[ 'token' ] = self.token
        response = self.vHandle.request( cmd, data )
        return response
    
    def testConnection( self ):
        return self._query( 'ping' )
    
    def hcp_getAgentStates( self, aid = None, hostname = None ):
        filters = {}
        if aid != None:
            filters[ 'agent_id' ] = aid
        if hostname != None:
            filters[ 'hostname' ] = hostname
        return self._query( 'hcp.get_agent_states', filters )
    
    def hcp_getPeriod( self ):
        return self._query( 'hcp.get_period' )
    
    def hcp_setPeriod( self, period ):
        return self._query( 'hcp.set_period', { 'period' : int( period ) } )
    
    def hcp_getEnrollmentRules( self ):
        return self._query( 'hcp.get_enrollment_rules' )
    
    def hcp_addEnrollmentRule( self, mask, externalIp, internalIp, newOrg, newSubnet, hostname ):
        return self._query( 'hcp.add_enrollment_rule', { 'mask' : mask,
                                                         'external_ip' : externalIp,
                                                         'internal_ip' : internalIp,
                                                         'new_org' : newOrg,
                                                         'new_subnet' : newSubnet,
                                                         'hostname' : hostname } )
    
    def hcp_delEnrollmentRule( self, mask, externalIp, internalIp, newOrg, newSubnet, hostname ):
        return self._query( 'hcp.del_enrollment_rule', { 'mask' : mask,
                                                         'external_ip' : externalIp,
                                                         'internal_ip' : internalIp,
                                                         'new_org' : newOrg,
                                                         'new_subnet' : newSubnet,
                                                         'hostname' : hostname } )
    
    def hcp_getTaskings( self ):
        return self._query( 'hcp.get_taskings' )
    
    def hcp_addTasking( self, mask, moduleId, hashStr ):
        return self._query( 'hcp.add_tasking', { 'mask' : mask, 'module_id' : int( moduleId ), 'hash' : hashStr } )
    
    def hcp_delTasking( self, mask, moduleId, hashStr ):
        return self._query( 'hcp.remove_tasking', { 'mask' : mask, 'module_id' : int( moduleId ), 'hash' : hashStr } )
    
    def hcp_getModules( self ):
        return self._query( 'hcp.get_modules' )
    
    def hcp_addModule( self, moduleId, binary, signature, description ):
        return self._query( 'hcp.add_module', { 'module_id' : moduleId, 'bin' : binary, 'signature' : signature, 'hash' : hashlib.sha256( binary ).hexdigest(), 'description' : description } )
    
    def hcp_delModule( self, moduleId, hashStr ):
        return self._query( 'hcp.remove_module', { 'module_id' : moduleId, 'hash' : hashStr } )
    
    def hcp_relocAgent( self, agentid, newOrg, newSubnet ):
        return self._query( 'hcp.reloc_agent', { 'agentid' : agentid,
                                                 'new_org' : newOrg,
                                                 'new_subnet' : newSubnet } )
    
    def hcp_getRelocations( self ):
        return self._query( 'hcp.get_relocations' )
    
    def hbs_getPeriod( self ):
        return self._query( 'hbs.get_period' )
    
    def hbs_setPeriod( self, period ):
        return self._query( 'hbs.set_period', { 'period' : int( period ) } )
    
    def hbs_getProfiles( self ):
        return self._query( 'hbs.get_profiles' )
    
    def hbs_addProfile( self, mask, config ):
        return self._query( 'hbs.set_profile', { 'mask' : mask, 'module_configs' : config } )
    
    def hbs_delProfile( self, mask ):
        return self._query( 'hbs.del_profile', { 'mask' : mask } )
    
    def hbs_taskAgent( self, toAgent, task, key, id, expiry = None, investigationId = None ):
        # Make sure it's a valid agentid
        a = AgentId( toAgent )
        if not a.isValid:
            return None
        if not type( task ) is rSequence:
            return None
        s = Signing( key )
        r = rpcm( isHumanReadable = True, isDebug = True )
        
        tags = Symbols()
        
        if investigationId is not None and '' != investigationId:
            task.addStringA( tags.hbs.INVESTIGATION_ID, investigationId )
        
        toSign = ( rSequence().addSequence( tags.base.HCP_ID, rSequence().addInt8( tags.base.HCP_ID_ORG, a.org )
                                                                         .addInt8( tags.base.HCP_ID_SUBNET, a.subnet )
                                                                         .addInt32( tags.base.HCP_ID_UNIQUE, a.unique )
                                                                         .addInt8( tags.base.HCP_ID_PLATFORM, a.platform )
                                                                         .addInt8( tags.base.HCP_ID_CONFIG, a.config ) )
                              .addSequence( tags.hbs.NOTIFICATION, task )
                              .addInt32( tags.hbs.NOTIFICATION_ID, id ) )
        if None != expiry:
            toSign.addTimestamp( tags.base.EXPIRY, int( expiry ) )
        toSign = r.serialise( toSign )
        sig = s.sign( toSign )
        
        final = r.serialise( rSequence().addBuffer( tags.base.BINARY, toSign )
                                        .addBuffer( tags.base.SIGNATURE, sig ) )
        
        return self._query( 'hbs.task_agent', { 'task' : final, 'agentid' : str( a ) } )
