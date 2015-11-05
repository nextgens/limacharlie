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

import readline
import cmd
import argparse
import shlex
import traceback
import json
import pprint
import time
from admin_lib import BEAdmin
from hcp_helpers import AgentId
from rpcm import rpcm
from rpcm import rSequence
from rpcm import rList
from Symbols import Symbols
from signing import Signing
import getpass
import pexpect
import os.path


def report_errors( func ):
    def silenceit( *args, **kwargs ):
        try:
            return func( *args,**kwargs )
        except:
            print( traceback.format_exc() )
            return None
    return( silenceit )

def hexArg( arg ):
    return int( arg, 16 )

class LP ( cmd.Cmd ):

    #===========================================================================
    #   HOUSEKEEPING
    #===========================================================================
    prompt = '<NEED_LOGIN> %> '

    def __init__( self ):
        cmd.Cmd.__init__( self )
        self.be = None
        self.user = None
        self.hbsKey = None
        self.aid = None
        self.investigationId = None
        self.tags = Symbols()
        readline.set_completer_delims(":;'\"? \t")

    def updatePrompt( self ):
        self.prompt = '%s%s / %s %s%%> ' % ( ( '' if self.hbsKey is None else '* ' ),
                                           ( '' if self.user is None else self.user ),
                                           ( '' if self.aid is None else self.aid ),
                                           ( '' if ( self.investigationId is None or self.investigationId == '' ) else ' : %s ' % self.investigationId ) )

    def getParser( self, desc, isHbsTask = False ):
        parser = argparse.ArgumentParser( prog = desc )

        if isHbsTask:
            parser.add_argument( '-!',
                                  type = AgentId,
                                  required = False,
                                  default = AgentId( self.aid ),
                                  help = 'agent id to change context to ONLY for the duration of this command.',
                                  dest = 'toAgent' )

            parser.add_argument( '-x',
                                  type = int,
                                  required = False,
                                  default = ( 60 * 60 * 1 ),
                                  help = 'set this command\'s specific expiry time in seconds.',
                                  dest = 'expiry' )

            parser.add_argument( '-@',
                                  type = str,
                                  required = False,
                                  default = self.investigationId,
                                  help = 'the investigation id to attach to the command, results and side-effects.',
                                  dest = 'investigationId' )

        return parser

    def parse( self, parser, line ):
        try:
            return parser.parse_args( shlex.split( line ) )
        except SystemExit:
            return None

    def do_exit( self, s ):
        return True

    def do_quit( self, s ):
        return True

    def emptyline( self ):
        pass

    def completedefault( self, text, line, begidx, endidx ):
        def get_possible_filename_completions(text):
            head, tail = os.path.split(text.strip())
            if head == "": #no head
                head = "."
            files = os.listdir(head)
            return [ os.path.join( head, f ) for f in files if f.startswith(tail) ]

        if begidx != 0:
            return get_possible_filename_completions( text )
        else:
            return [ x[ 3 : ] for x in dir( self ) if x.startswith( 'do_' ) ]

    def execAndPrintResponse( self, command, arguments, isHbsTask = False ):
        if isHbsTask:
            tmp = arguments
            arguments = argparse.Namespace()

            if not tmp.toAgent.isValid or self.hbsKey is None:
                print( 'Agent id and hbs key must be set in context.' )
                return

            if not hasattr( tmp, 'key' ) or tmp.key is None:
                setattr( tmp, 'key', self.hbsKey )

            if ( tmp.investigationId is not None ) and '' != tmp.investigationId:
                setattr( arguments, 'investigationId', tmp.investigationId )

            setattr( arguments, 'toAgent', tmp.toAgent )
            setattr( arguments, 'task', tmp.task )
            setattr( arguments, 'key', tmp.key )
            setattr( arguments, 'id', tmp.id )
            setattr( arguments, 'expiry', int( tmp.expiry + time.time() ) )

            del( tmp )

        for k, a in vars( arguments ).iteritems():
            if type( a ) is AgentId:
                if not a.isValid:
                    print( 'Invalid agent id.' )
                    return
                else:
                    setattr( arguments, k, str( a ) )

        results = command( **vars( arguments ) )

        if results.isSuccess:
            print( "<<<SUCCESS>>>" )
        elif results.isTimedOut:
            print( "<<<TIMEOUT>>>" )
        else:
            if 0 == len( results.error ):
                print( "<<<FAILURE>>>" )
            else:
                print( "<<<FAILURE: %s>>>" % results.error )
        pprint.pprint( results.data, indent = 2, width = 80 )

        return results.data

    def getTags( self, tagsPath ):
        raw_tags = json.loads( open( tagsPath, 'r' ).read() )
        tags = Symbols()
        for group in raw_tags[ 'groups' ]:
            g = Symbols()
            for definition in group[ 'definitions' ]:
                setattr( g, definition[ 'name' ], str( definition[ 'value' ] ) )
            setattr( tags, group[ 'groupName' ], g )

        return tags

    #===========================================================================
    #   SESSION SETTING COMMANDS
    #===========================================================================
    @report_errors
    def do_login( self, s ):
        '''Login to the BE using credentials stored in a config file.'''

        parser = self.getParser( 'login' )
        parser.add_argument( 'configFile',
                             type = argparse.FileType( 'r' ),
                             help = 'config file specifying the endpoint and token to use' )
        parser.add_argument( '-k', '--key',
                             required = False,
                             default = None,
                             #type = argparse.FileType( 'r' ),
                             type = str,
                             help = 'key to use to sign hbs tasks',
                             dest = 'key' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            try:
                config = json.loads( arguments.configFile.read() )
            except:
                print( "Invalid config file format (JSON): %s" % traceback.format_exc() )
                return

            if 'beach_config' not in config or 'token' not in config:
                print( "Missing endpoint or token in config." )
                return

            _ = os.getcwd()
            os.chdir( os.path.dirname(  __file__ ) )
            self.be = BEAdmin( config[ 'beach_config' ], config[ 'token' ] )
            os.chdir( _ )

            remoteTime = self.be.testConnection()

            if remoteTime.isTimedOut:
                print( "Endpoint did not respond." )
                return

            if 'pong' not in remoteTime.data:
                print( "Endpoint responded with invalid data." )
                return

            if arguments.key is not None:
                if os.path.isfile( arguments.key ):
                    try:
                        password = getpass.getpass()
                        print( "...decrypting key..." )
                        # There are weird problems with pexpect and newlines and binary, so
                        # we have to brute force it a bit
                        for i in range( 0, 30 ):
                            proc = pexpect.spawn( 'openssl aes-256-cbc -d -in %s' % arguments.key )
                            proc.expect( [ 'enter aes-256-cbc decryption password: *' ] )
                            proc.sendline( password )
                            proc.expect( "\r\n" )
                            proc.expect( ".*" )
                            self.hbsKey = proc.match.group( 0 ).replace( "\r\n", "\n" )
                            try:
                                testSign = Signing( self.hbsKey )
                                testSig = testSign.sign( 'a' )
                                if testSig is not None:
                                    break
                            except:
                                self.hbsKey = None

                        if self.hbsKey is not None:
                            print( "success, authenticated!" )
                        else:
                            print( "error loading key, bad key format or password?" )
                    except:
                        self.hbsKey = None
                        print( "error getting cloud key: %s" % traceback.format_exc() )
                    if self.hbsKey is not None and 'bad decrypt' in self.hbsKey:
                        print( "Invalid password" )
                        self.hbsKey = None
                else:
                    print( "Invalid key file: %s." % arguments.key )
                    self.hbsKey = None
            else:
                self.hbsKey = None

            remoteTime = remoteTime.data.get( 'pong', 0 )
            print( "Successfully logged in." )
            print( "Remote endpoint time: %s." % remoteTime )

            self.user = config[ 'token' ].split( '/' )[ 0 ]

            self.updatePrompt()

    @report_errors
    def do_chid( self, s ):
        '''Change execution context to a specific agent id, used only for HBS tasking.'''

        parser = self.getParser( 'chid' )
        parser.add_argument( 'aid',
                             type = AgentId,
                             help = 'agent id to change context to' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            aid = arguments.aid
            if aid.isValid:
                self.aid = aid
                self.updatePrompt()
            else:
                print( 'Agent Id is not valid.' )

    @report_errors
    def do_setInvestigationId( self, s ):
        '''Change execution context to relate to a specific investigationId, used only for HBS tasking.'''

        parser = self.getParser( 'setInvestigationId' )
        parser.add_argument( 'investigationId',
                             type = str,
                             help = 'investigation id to change context to' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.investigationId = arguments.investigationId
            self.updatePrompt()



    #===========================================================================
    #   HCP COMMANDS
    #===========================================================================
    @report_errors
    def do_hcp_getAgentStates( self, s ):
        '''Get the general state of agents.'''

        parser = self.getParser( 'getAgentStates' )
        parser.add_argument( '-a', '--aid',
                              type = AgentId,
                              required = False,
                              help = 'agent id to retrieve the info of',
                              dest = 'aid' )
        parser.add_argument( '-n', '--hostname',
                              type = str,
                              required = False,
                              help = 'hostname of the agent to retrieve the info of',
                              dest = 'hostname' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_getAgentStates, arguments )

    @report_errors
    def do_hcp_setPeriod( self, s ):
        '''Set the period agents beacon back.'''

        parser = self.getParser( 'setPeriod' )
        parser.add_argument( 'period',
                             type = int,
                             help = 'new period to schedule hcp beacons over' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_setPeriod, arguments )

    @report_errors
    def do_hcp_getPeriod( self, s ):
        '''Get the current agent beacon period.'''

        parser = self.getParser( 'getPeriod' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_getPeriod, arguments )

    @report_errors
    def do_hcp_addEnrollmentRule( self, s ):
        '''Add a new enrollment rule for new agents.'''

        parser = self.getParser( 'addEnrollmentRule' )
        parser.add_argument( '-m', '--mask',
                             type = AgentId,
                             required = True,
                             help = 'agent id mask this rule applies to',
                             dest = 'mask' )
        parser.add_argument( '-i', '--internalip',
                             type = str,
                             required = False,
                             default = '255.255.255.255',
                             help = 'internal ip mask the rule applies to (255 wildcard)',
                             dest = 'internalIp' )
        parser.add_argument( '-e', '--externalip',
                             type = str,
                             required = False,
                             default = '255.255.255.255',
                             help = 'external ip mask the rule applies to (255 wildcard)',
                             dest = 'externalIp' )
        parser.add_argument( '-s', '--newsubnet',
                             type = hexArg,
                             required = True,
                             help = 'new subnet to give to agents matching this rule (hex)',
                             dest = 'newSubnet' )
        parser.add_argument( '-o', '--neworg',
                             type = hexArg,
                             required = True,
                             help = 'new org to give to agents matching this rule (hex)',
                             dest = 'newOrg' )
        parser.add_argument( '-n', '--hostname',
                             type = str,
                             required = False,
                             default = '',
                             help = 'hostname of the host',
                             dest = 'hostname' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_addEnrollmentRule, arguments )

    @report_errors
    def do_hcp_delEnrollmentRule( self, s ):
        '''Remove an enrollment rule for new agents.'''

        parser = self.getParser( 'addEnrollmentRule' )
        parser.add_argument( '-m', '--mask',
                             type = AgentId,
                             required = True,
                             help = 'agent id mask this rule applies to',
                             dest = 'mask' )
        parser.add_argument( '-i', '--internalip',
                             type = str,
                             required = False,
                             default = '255.255.255.255',
                             help = 'internal ip mask the rule applies to (255 wildcard)',
                             dest = 'internalIp' )
        parser.add_argument( '-e', '--externalip',
                             type = str,
                             required = False,
                             default = '255.255.255.255',
                             help = 'external ip mask the rule applies to (255 wildcard)',
                             dest = 'externalIp' )
        parser.add_argument( '-s', '--newsubnet',
                             type = hexArg,
                             required = True,
                             help = 'new subnet to give to agents matching this rule (hex)',
                             dest = 'newSubnet' )
        parser.add_argument( '-o', '--neworg',
                             type = hexArg,
                             required = True,
                             help = 'new org to give to agents matching this rule (hex)',
                             dest = 'newOrg' )
        parser.add_argument( '-n', '--hostname',
                             type = str,
                             required = False,
                             default = '',
                             help = 'hostname of the host',
                             dest = 'hostname' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_delEnrollmentRule, arguments )

    @report_errors
    def do_hcp_getEnrollmentRules( self, s ):
        '''Get the list of enrollment rules for new agents.'''

        parser = self.getParser( 'getPeriod' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_getEnrollmentRules, arguments )

    @report_errors
    def do_hcp_addTasking( self, s ):
        '''Task a module to a list of agents.'''

        parser = self.getParser( 'addTasking' )
        parser.add_argument( '-m', '--mask',
                             type = AgentId,
                             required = True,
                             help = 'agent id mask of the rule',
                             dest = 'mask' )
        parser.add_argument( '-i', '--moduleid',
                             type = int,
                             required = True,
                             help = 'module id to task',
                             dest = 'moduleId' )
        parser.add_argument( '-s', '--hash',
                             type = str,
                             required = True,
                             help = 'hash of the module to task',
                             dest = 'hashStr' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_addTasking, arguments )

    @report_errors
    def do_hcp_delTasking( self, s ):
        '''Remove a module from tasking to agents.'''

        parser = self.getParser( 'delTasking' )
        parser.add_argument( '-m', '--mask',
                             type = AgentId,
                             required = True,
                             help = 'agent id mask of the rule',
                             dest = 'mask' )
        parser.add_argument( '-i', '--moduleid',
                             type = int,
                             required = True,
                             help = 'module id tasked',
                             dest = 'moduleId' )
        parser.add_argument( '-s', '--hash',
                             type = str,
                             required = True,
                             help = 'hash of the module to untask',
                             dest = 'hashStr' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_delTasking, arguments )

    @report_errors
    def do_hcp_getTaskings( self, s ):
        '''Get the list of modules tasked to agents.'''

        parser = self.getParser( 'getTaskings' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_getTaskings, arguments )

    @report_errors
    def do_hcp_addModule( self, s ):
        '''Add a taskable module.'''

        parser = self.getParser( 'addModule' )
        parser.add_argument( '-i', '--moduleid',
                             type = int,
                             required = True,
                             help = 'module id',
                             dest = 'moduleId' )
        parser.add_argument( '-b', '--binary',
                             type = argparse.FileType( 'r' ),
                             required = True,
                             help = 'path to file containing module',
                             dest = 'binary' )
        parser.add_argument( '-s', '--signature',
                             type = argparse.FileType( 'r' ),
                             required = True,
                             help = 'path to file containing signature of the module',
                             dest = 'signature' )
        parser.add_argument( '-d', '--description',
                             type = str,
                             required = True,
                             help = 'description of the module',
                             dest = 'description' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            arguments.binary = arguments.binary.read()
            arguments.signature = arguments.signature.read()
            self.execAndPrintResponse( self.be.hcp_addModule, arguments )

    @report_errors
    def do_hcp_delModule( self, s ):
        '''Remove a taskable module.'''

        parser = self.getParser( 'delTasking' )
        parser.add_argument( '-i', '--moduleid',
                             type = int,
                             required = True,
                             help = 'module id',
                             dest = 'moduleId' )
        parser.add_argument( '-s', '--hash',
                             type = str,
                             required = True,
                             help = 'hash of the module',
                             dest = 'hashStr' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_delModule, arguments )

    @report_errors
    def do_hcp_getModules( self, s ):
        '''Get the list of modules available for tasking.'''

        parser = self.getParser( 'getModules' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_getModules, arguments )

    @report_errors
    def do_hcp_relocAgent( self, s ):
        '''Relocate an agent to a new org and network.'''

        parser = self.getParser( 'relocAgent' )
        parser.add_argument( '-a', '--agentid',
                             type = AgentId,
                             required = True,
                             help = 'agent id to relocate',
                             dest = 'agentid' )
        parser.add_argument( '-s', '--newsubnet',
                             type = hexArg,
                             required = True,
                             help = 'new subnet to give to agents matching this rule (hex)',
                             dest = 'newSubnet' )
        parser.add_argument( '-o', '--neworg',
                             type = hexArg,
                             required = True,
                             help = 'new org to give to agents matching this rule (hex)',
                             dest = 'newOrg' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_relocAgent, arguments )

    @report_errors
    def do_hcp_getRelocations( self, s ):
        '''Get the list of agent reolcations.'''

        parser = self.getParser( 'getRelocations' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hcp_getRelocations, arguments )





    #===========================================================================
    #   HBS COMMANDS
    #===========================================================================
    def do_hbs_setPeriod( self, s ):
        '''Set the period agents beacon back.'''

        parser = self.getParser( 'setPeriod' )
        parser.add_argument( 'period',
                             type = int,
                             help = 'new period to schedule hcp beacons over' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hbs_setPeriod, arguments )

    @report_errors
    def do_hbs_getPeriod( self, s ):
        '''Get the current agent beacon period.'''

        parser = self.getParser( 'getPeriod' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hbs_getPeriod, arguments )

    @report_errors
    def do_hbs_addProfile( self, s ):
        '''Add an execution profile.'''

        parser = self.getParser( 'addProfile' )
        parser.add_argument( '-m', '--mask',
                             type = AgentId,
                             required = True,
                             help = 'agent id mask the profile applies to',
                             dest = 'mask' )
        parser.add_argument( '-f', '--configfile',
                             type = argparse.FileType( 'r' ),
                             required = True,
                             help = 'path to the file containing the config',
                             dest = 'config' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            arguments.config = arguments.config.read()
            self.execAndPrintResponse( self.be.hbs_addProfile, arguments )

    @report_errors
    def do_hbs_delProfile( self, s ):
        '''Remove an execution profile.'''

        parser = self.getParser( 'delProfile' )
        parser.add_argument( '-m', '--mask',
                             type = AgentId,
                             required = True,
                             help = 'agent id mask the profile applies to',
                             dest = 'mask' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hbs_delProfile, arguments )

    @report_errors
    def do_hbs_getProfiles( self, s ):
        '''Get the list of profiles.'''

        parser = self.getParser( 'getProfiles' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            self.execAndPrintResponse( self.be.hbs_getProfiles, arguments )

    #===========================================================================
    #   HBS TASKINGS
    #===========================================================================
    def _executeHbsTasking( self, notifId, payload, arguments ):
        setattr( arguments, 'id', notifId )
        setattr( arguments, 'task', payload )

        # Multiplex to all agents matching
        getAgentsArg = argparse.Namespace()
        setattr( getAgentsArg, 'aid', arguments.toAgent )
        agents = self.execAndPrintResponse( self.be.hcp_getAgentStates, getAgentsArg )
        if agents is not None:
            if 'agents' in agents:
                for aid in agents[ 'agents' ].keys():
                    print( "Tasking agent %s" % aid )
                    arguments.toAgent = AgentId( aid )
                    self.execAndPrintResponse( self.be.hbs_taskAgent, arguments, True )
            else:
                print( "No matching agents found." )
        else:
            print( "Failed to get agent list from endpoint." )

    @report_errors
    def do_file_get( self, s ):
        '''Retrieve a file from the host.'''

        parser = self.getParser( 'file_get', True )
        parser.add_argument( 'file',
                             type = unicode,
                             help = 'file path to file to get' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.FILE_GET_REQ,
                                     rSequence().addStringW( self.tags.base.FILE_PATH, arguments.file ),
                                     arguments )

    @report_errors
    def do_file_info( self, s ):
        '''Retrieve information on a file from the host.'''

        parser = self.getParser( 'file_info', True )
        parser.add_argument( 'file',
                             type = unicode,
                             help = 'file path to file to get info on' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.FILE_INFO_REQ,
                                     rSequence().addStringW( self.tags.base.FILE_PATH, arguments.file ),
                                     arguments )

    @report_errors
    def do_dir_list( self, s ):
        '''Get the directory listing.'''

        parser = self.getParser( 'dir_list', True )
        parser.add_argument( 'rootDir',
                             type = unicode,
                             help = 'the root directory where to begin the listing from' )
        parser.add_argument( 'fileExp',
                             type = unicode,
                             help = 'a file name expression supporting basic wildcards like * and ?' )
        parser.add_argument( '-d', '--depth',
                             dest = 'depth',
                             required = False,
                             default = 0,
                             help = 'optional maximum depth of the listing, defaults to a single level' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.hbs.NOTIFICATION_DIR_LIST_REQ,
                                     rSequence().addStringW( self.tags.base.FILE_PATH, arguments.fileExp )
                                                .addStringW( self.tags.base.DIRECTORY_PATH, arguments.rootDir )
                                                .addInt32( self.tags.base.DIRECTORY_LIST_DEPTH, arguments.depth ),
                                     arguments )

    @report_errors
    def do_file_del( self, s ):
        '''Delete a file from the host.'''

        parser = self.getParser( 'file_del', True )
        parser.add_argument( 'file',
                             type = unicode,
                             help = 'file path to delete' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.FILE_DEL_REQ,
                                     rSequence().addStringW( self.tags.base.FILE_PATH, arguments.file ),
                                     arguments )

    @report_errors
    def do_file_mov( self, s ):
        '''Move a file on the host.'''

        parser = self.getParser( 'file_mov', True )
        parser.add_argument( 'srcFile',
                             type = unicode,
                             help = 'source file path' )
        parser.add_argument( 'dstFile',
                             type = unicode,
                             help = 'destination file path' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.FILE_MOV_REQ,
                                     rSequence().addStringW( self.tags.base.FILE_PATH, arguments.srcFile )
                                                .addStringW( self.tags.base.FILE_NAME, arguments.dstFile ),
                                     arguments )

    @report_errors
    def do_file_hash( self, s ):
        '''Hash a file from the host.'''

        parser = self.getParser( 'file_hash', True )
        parser.add_argument( 'file',
                             type = unicode,
                             help = 'file path to hash' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.FILE_HASH_REQ,
                                     rSequence().addStringW( self.tags.base.FILE_PATH, arguments.file ),
                                     arguments )

    @report_errors
    def do_mem_map( self, s ):
        '''Get the memory mapping of a specific process.'''

        parser = self.getParser( 'mem_map', True )
        parser.add_argument( 'pid',
                             type = int,
                             help = 'pid of the process to get the map from' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.MEM_MAP_REQ,
                                     rSequence().addInt32( self.tags.base.PROCESS_ID, arguments.pid ),
                                     arguments )

    @report_errors
    def do_mem_read( self, s ):
        '''Read the memory of a process at a specific address.'''

        parser = self.getParser( 'mem_read', True )
        parser.add_argument( 'pid',
                             type = int,
                             help = 'pid of the process to get the map from' )
        parser.add_argument( 'baseAddr',
                             type = hexArg,
                             help = 'base address to read from, in HEX FORMAT' )
        parser.add_argument( 'memSize',
                             type = hexArg,
                             help = 'number of bytes to read, in HEX FORMAT' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.MEM_READ_REQ,
                                     rSequence().addInt32( self.tags.base.PROCESS_ID, arguments.pid )
                                                .addInt64( self.tags.base.BASE_ADDRESS, arguments.baseAddr )
                                                .addInt32( self.tags.base.MEMORY_SIZE, arguments.memSize ),
                                     arguments )

    @report_errors
    def do_mem_handles( self, s ):
        '''Get the handles openned by a specific process.'''

        parser = self.getParser( 'mem_handles', True )
        parser.add_argument( 'pid',
                             type = int,
                             help = 'pid of the process to get the handles from, 0 for all processes' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.MEM_HANDLES_REQ,
                                     rSequence().addInt32( self.tags.base.PROCESS_ID, arguments.pid ),
                                     arguments )

    @report_errors
    def do_mem_strings( self, s ):
        '''Get the strings from a specific process.'''

        parser = self.getParser( 'mem_strings', True )
        parser.add_argument( 'pid',
                             type = int,
                             help = 'pid of the process to get the strings from' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.MEM_STRINGS_REQ,
                                     rSequence().addInt32( self.tags.base.PROCESS_ID, arguments.pid ),
                                     arguments )

    @report_errors
    def do_os_services( self, s ):
        '''Get the services registered on the host.'''

        parser = self.getParser( 'getServices', True )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.OS_SERVICES_REQ,
                                     rSequence(),
                                     arguments )

    @report_errors
    def do_os_drivers( self, s ):
        '''Get the drivers registered on the host.'''

        parser = self.getParser( 'os_drivers', True )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.OS_DRIVERS_REQ,
                                     rSequence(),
                                     arguments )

    @report_errors
    def do_os_kill_process( self, s ):
        '''Kill a process on the host.'''

        parser = self.getParser( 'os_kill_process', True )
        parser.add_argument( 'pid',
                             type = int,
                             help = 'pid of the process to kill' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.OS_KILL_PROCESS_REQ,
                                     rSequence().addInt32( self.tags.base.PROCESS_ID, arguments.pid ),
                                     arguments )

    @report_errors
    def do_os_processes( self, s ):
        '''Generate a new process snapshot.'''

        parser = self.getParser( 'os_processes', True )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.OS_PROCESSES_REQ,
                                     rSequence(),
                                     arguments )

    @report_errors
    def do_os_autoruns( self, s ):
        '''Generate a new autoruns snapshot.'''

        parser = self.getParser( 'os_autoruns', True )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.OS_AUTORUNS_REQ,
                                     rSequence(),
                                     arguments )

    @report_errors
    def do_mem_find_string( self, s ):
        '''Find the specific strings in a specific process.'''

        parser = self.getParser( 'mem_find_string', True )
        parser.add_argument( 'pid',
                             type = int,
                             help = 'pid of the process to search in' )
        parser.add_argument( '-s', '--strings',
                             type = unicode,
                             required = True,
                             nargs = '*',
                             dest = 'strings',
                             help = 'list of strings to look for' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            seq = rSequence().addInt32( self.tags.base.PROCESS_ID, arguments.pid )
            l = rList()
            for s in arguments.strings:
                l.addStringW( self.tags.base.STRING, s )
            seq.addList( self.tags.base.STRINGSW, l )
            self._executeHbsTasking( self.tags.notification.MEM_FIND_STRING_REQ,
                                     seq,
                                     arguments )

    @report_errors
    def do_mem_find_handle( self, s ):
        '''Find the handles in any process that contain a specific substring.'''

        parser = self.getParser( 'mem_find_handle', True )
        parser.add_argument( 'needle',
                             type = unicode,
                             help = 'substring of the handle names to get' )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.MEM_FIND_HANDLE_REQ,
                                     rSequence().addStringW( self.tags.base.HANDLE_NAME, arguments.needle ),
                                     arguments )

    @report_errors
    def do_run_script( self, s ):
        '''Runs a list of commands from a file but with additional context passed in the command line.'''

        parser = self.getParser( 'run_script' )
        parser.add_argument( '-f', '--configfile',
                             type = argparse.FileType( 'r' ),
                             required = True,
                             help = 'path to the file containing the script to run',
                             dest = 'script' )
        parser.add_argument( '-n', '--nocontext',
                             dest = 'nocontext',
                             action = 'store_true',
                             default = False,
                             required = False,
                             help = 'if present will NOT overwrite the context in the script with the one passed in the run_script command line' )
        arguments = self.parse( parser, s )

        if arguments is not None:
            arguments.script = [ x for x in arguments.script.read().split( '\n' ) if ( x.strip() != '' and not x.startswith( '#' ) ) ]

            print( "Executing script containing %d commands." % ( len( arguments.script ), ) )

            if arguments.nocontext:
                curContext = ''
            else:
                curContext = ' -! %s -@ %s -x %s ' % ( arguments.toAgent, arguments.investigationId, arguments.expiry )

            self.cmdqueue.extend( [ x + curContext for x in arguments.script ] )

    @report_errors
    def do_history_dump( self, s ):
        '''Dump the full recent history of events on the sensor.'''

        parser = self.getParser( 'history_dump', True )
        arguments = self.parse( parser, s )
        if arguments is not None:
            self._executeHbsTasking( self.tags.notification.HISTORY_DUMP_REQ,
                                     rSequence(),
                                     arguments )

g_parser = argparse.ArgumentParser()

g_parser.add_argument( '--script',
                        type = argparse.FileType('r'),
                        required = False,
                        default = None,
                        help = 'execute the script of commands then exit.',
                        dest = 'script' )

g_args = g_parser.parse_args()

lp = LP()

if g_args.script is not None:
    for line in g_args.script:
        lp.onecmd( line )
else:
    lp.cmdloop( '''
    ====================
    RPHCP Interactive CLI
    (c) refractionPOINT 2015
    ====================
    ''' )

