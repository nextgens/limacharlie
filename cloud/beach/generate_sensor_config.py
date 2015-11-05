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
from hcp.hcp_helpers import AgentId

def int16( s ):
    return int( s, 16 )

parser = argparse.ArgumentParser()
parser.add_argument( '-p', '--primary',
                     type = str,
                     required = True,
                     dest = 'primary',
                     help = 'primary URL for beacons' )
parser.add_argument( '-s', '--secondary',
                     type = str,
                     required = True,
                     dest = 'secondary',
                     help = 'secondary URL for beacons' )
parser.add_argument( '-o', '--default-org',
                     type = int16,
                     required = True,
                     dest = 'org',
                     help = 'default sensor id org' )
parser.add_argument( '-u', '--default-subnet',
                     type = int16,
                     required = True,
                     dest = 'subnet',
                     help = 'default sensor id subnet' )
parser.add_argument( '-c', '--c2-key',
                     type = str,
                     required = True,
                     dest = 'c2',
                     help = 'file path to c2 public key' )
parser.add_argument( '-r', '--root-key',
                     type = str,
                     required = True,
                     dest = 'root',
                     help = 'file path to root public key' )

args = parser.parse_args()

config = '''
rSequence().addStringA( _.hcp.PRIMARY_URL, "%s" )
           .addStringA( _.hcp.SECONDARY_URL, "%s" )
           .addSequence( _.base.HCP_ID, rSequence().addInt8( _.base.HCP_ID_ORG, %d )
                                                   .addInt8( _.base.HCP_ID_SUBNET, %d )
                                                   .addInt32( _.base.HCP_ID_UNIQUE, 0 )
                                                   .addInt8( _.base.HCP_ID_PLATFORM, 0 )
                                                   .addInt8( _.base.HCP_ID_CONFIG, 0 ) )
           .addBuffer( _.hcp.C2_PUBLIC_KEY, open( '%s', 'r' ).read() )
           .addBuffer( _.hcp.ROOT_PUBLIC_KEY, open( '%s', 'r' ).read() )
''' % ( args.primary,
        args.secondary,
        args.org,
        args.subnet,
        args.c2,
        args.root )

print( config )