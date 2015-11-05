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

import traceback
import M2Crypto
import hashlib
import base64



class Signing( object ):
    
    def __init__( self, privateKey ):
        self.pri_key = None
        if not privateKey.startswith( '-----BEGIN RSA PRIVATE KEY-----' ):
            privateKey = self.der2pem( privateKey )
            
        self.pri_key = M2Crypto.RSA.load_key_string( privateKey )
    
    
    def sign( self, buff ):
        sig = None
        h = hashlib.sha256( buff ).digest()
        
        sig = self.pri_key.private_encrypt( h, M2Crypto.RSA.pkcs1_padding )
        
        return sig
    
    def der2pem( self, der ):
        encoded = base64.b64encode( der )
        encoded = [ encoded[ i : i + 64 ] for i in range( 0, len( encoded ), 64 ) ]
        encoded = '\n'.join( encoded )
        pem = '-----BEGIN RSA PRIVATE KEY-----\n%s\n-----END RSA PRIVATE KEY-----\n' % encoded
        
        return pem
    

if __name__ == '__main__':
    import sys
    import argparse

    parser = argparse.ArgumentParser( description = 'Signing tool' )
    parser.add_argument( '-k', '--key',
                         type = argparse.FileType( 'r' ),
                         required = True,
                         help = 'private key to use to sign file',
                         dest = 'private_key' )
    parser.add_argument( '-f', '--file',
                         type = argparse.FileType( 'r' ),
                         required = True,
                         help = 'file to sign',
                         dest = 'file' )
    parser.add_argument( '-o', '--output',
                         type = str,
                         required = False,
                         help = 'path to output signature file, default is <input file>.sig',
                         dest = 'output_file' )
    
    arguments = parser.parse_args()
    
    output_file = '%s.sig' % arguments.file.name
    if None != arguments.output_file:
        output_file = arguments.output_file
    
    try:
        s = Signing( arguments.private_key.read() )
    except:
        print( "Error loading key: %s" % traceback.format_exc() )
        sys.exit()
    
    try:
        sig = s.sign( arguments.file.read() )
    except:
        print( "Error signing file: %s" % traceback.format_exc() )
        sys.exit()
    
    try:
        f = open( output_file, 'w' )
        f.write( sig )
        f.close()
    except:
        print( "Error writing signature to output file: %s" % traceback.format_exc() )
        sys.exit()
    
    print( "Signing file %s with key %s to file %s." % ( arguments.file.name, arguments.private_key.name, output_file ) )