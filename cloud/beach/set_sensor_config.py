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

from hcp.rpcm import rpcm
from hcp.rpcm import rSequence
from hcp.rpcm import rList
from hcp.Symbols import Symbols
from hcp.hcp_helpers import AgentId

import os
import sys

# This is the key also defined in the sensor as _HCP_DEFAULT_STATIC_STORE_KEY
# and used with the same algorithm as obfuscationLib
OBFUSCATION_KEY = "\xFA\x75\x01"
STATIC_STORE_MAX_SIZE = 1024 * 10

def obfuscate( buffer, key ):
    obf = ''
    index = 0
    for hx in buffer:
        obf = obf + chr( ( ( ord( key[ index % len( key ) ] ) ^ ( index % 255 ) ) ^ ( STATIC_STORE_MAX_SIZE % 255 ) ) ^ ord( hx ) )
        index = index + 1
    return obf

if 3 != len( sys.argv ):
    print( "Usage: set_sensor_config.py configFile sensorExec" )
    sys.exit()

configFile = open( sys.argv[ 1 ], 'r' ).read()
sensorFile = open( sys.argv[ 2 ], 'r' )
sensor = sensorFile.read()
sensorFile.close()

prevPath = os.getcwd()
os.chdir( os.path.join( os.path.dirname( __file__ ), '..', '..' ) )

r = rpcm( isDebug = True )
rpcm_environment = { '_' : Symbols(), 'rList' : rList, 'rSequence' : rSequence, 'AgentId' : AgentId }

conf = eval( configFile.replace( '\n', '' ), rpcm_environment )

conf = obfuscate( r.serialise( conf ), OBFUSCATION_KEY )

magic = "\xFA\x57\xF0\x0D" + ( "\x00" * ( len( conf ) - 4 ) )

os.chdir( prevPath )

if magic in sensor:
    sensor = sensor.replace( magic, conf )
    sensorFile = open( sys.argv[ 2 ], 'w' )
    sensorFile.write( sensor )
    sensorFile.close()
    print( "Sensor patched." )
else:
    print( "Sensor ALREADY PATCHED." )
    sys.exit( -1 )

