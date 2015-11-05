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

import argparse
from hcp.Symbols import Symbols
from hcp.hcp_helpers import HbsCollectorId

tags = Symbols()

def validNotification( s ):
    if not hasattr( tags.notification, s ):
        raise argparse.ArgumentError( 'value is not a valid notification tag' )
    return s

def int16( s ):
    return int( s, 16 )

parser = argparse.ArgumentParser()
parser.add_argument( '-x', '--exfil',
                     type = validNotification,
                     required = True,
                     dest = 'exfil',
                     nargs = '+',
                     help = 'list of events to exfil back to the cloud' )

parser.add_argument( '-d', '--disable',
                     type = int,
                     required = False,
                     dest = 'disable',
                     nargs = '+',
                     help = 'list of collector id to disable' )

parser.add_argument( '--hidden-module-delta',
                     type = str,
                     required = False,
                     dest = 'hidden_delta',
                     default = None,
                     help = 'expression of number of seconds between checks for hidden modules' )

parser.add_argument( '--os-snapshots-delta',
                     type = str,
                     required = False,
                     dest = 'os_delta',
                     default = None,
                     help = 'expression of number of seconds between os snapshots (processes, modules, services, etc)' )

args = parser.parse_args()

collectors = {}

profile_root = 'rList()'

profile = '''
rList().addSequence( _.hbs.CONFIGURATION,
                     rSequence().addInt32( _.hbs.CONFIGURATION_ID, 0 )
                                .addList( _.hbs.LIST_NOTIFICATIONS, rList()%s)%s
'''

exfilEvents = []
if args.exfil is not None:
    for e in args.exfil:
        exfilEvents.append( '.addInt32( _.hbs.NOTIFICATION_ID, _.notification.%s )' % ( e, ) )
collectors.setdefault( HbsCollectorId.EXFIL, {} )[ 'to_exfil' ] = exfilEvents

if args.disable is not None:
    for d in args.disable:
        collectors.setdefault( d, {} )[ 'to_disable' ] = True

if args.hidden_delta is not None:
    collectors.setdefault( HbsCollectorId.HIDDEN_MODULE, {} )[ 'delta' ] = args.hidden_delta

if args.os_delta is not None:
    collectors.setdefault( HbsCollectorId.OS_FORENSIC, {} )[ 'delta' ] = args.os_delta

for collector, info in collectors.iteritems():
    profile = '.addSequence( _hbs.CONFIGURATION, rSequence().addInt32( _.hbs.CONFIGURATION_ID, HbsCollectorId.%s )' % HbsCollectorId.lookup[ collector ]
    if collector ==  HbsCollectorId.EXFIL and 'to_exfil' in info:
        profile += '.addList( _.hbs.LIST_NOTIFICATIONS, rList()%s)' % ''.join( info[ 'to_exfil' ] )

    if 'to_disable' in info:
        profile += '.addInt8( _.base.IS_DISABLED, 1 )'

    if 'delta' in info:
        profile += '.addTimedelta( _.base.TIMEDELTA, %s )' % info[ 'delta' ]

    profile += ')\n'
    profile_root += profile

print( profile_root )