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

import M2Crypto
import sys
import os


if len( sys.argv ) < 2:
    print( 'This takes a single argument: keyName' )
    sys.exit()

r = M2Crypto.RSA.gen_key( 2048, 65537 )

r.save_pub_key( '%s.pub.pem' % sys.argv[ 1 ] )

r.save_key( '%s.priv.pem' % sys.argv[ 1 ], None )

r.save_key_der( '%s.priv.der' % sys.argv[ 1 ] )

#rp = M2Crypto.RSA.new_pub_key( r.pub() )
#rp.save_key_der( '%s.pub.der' % sys.argv[ 1 ] )

os.system( 'openssl rsa -in %s.priv.pem -out %s.pub.der -outform DER -pubout' % ( sys.argv[ 1 ], sys.argv[ 1 ] ) )

c = open( '%s.pub.der' % sys.argv[ 1 ], 'r' ).read()
with open( '%s.pub.carray' % sys.argv[ 1 ], 'w' ) as f:
    f.write( '{\n    ' )
    for i in range( 0, len( c ) ):
        val = hex( ord( c[ i ] ) )[ 2 : ]
        if 1 == len( val ):
            val = '0%s' % val
        f.write( '0x%s' % val )
        if i != len( c ) - 1:
            f.write( ', ' )
        if i != 0 and 0 == ( ( i + 1 ) % 12 ):
            f.write( '\n    ' )
    f.write( '\n}\n' )

