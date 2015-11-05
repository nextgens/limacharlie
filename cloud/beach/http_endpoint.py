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

from gevent import wsgi
import urlparse

import os
import sys
from beach.beach_api import Beach

if 2 != len( sys.argv ):
    print( "Usage: http_endpoint.py beachConfigFile" )
    sys.exit(-1)

def handle_beacon( environment, start_response ):
    global vHandle
    isSuccess = False
    response = ''
    status = '400 Bad Request'

    try:
        params = [ x for x in environment[ 'wsgi.input' ].read() ]

        params = urlparse.parse_qs( ''.join( params ).lstrip( '&' ), strict_parsing = True )
    except:
        params = {}

    if 'pl' in params:
        clean_params = { 'payload' : params[ 'pl' ][ 0 ],
                         'public_ip' : str( environment.get( 'X-FORWARDED-FOR',
                                                        environment.get( 'REMOTE_ADDR', '' ) ) ) }

        resp = vHandle.request( 'beacon', data = clean_params )

        if resp.isSuccess:
            isSuccess = True
            response = '<html><h1>%s</h1></html>' % resp.data[ 'resp' ]

    if isSuccess:
        status = '200 OK'

    start_response( status, [ ( 'Content-Type', 'text/html' ),
                              ( 'Content-Length', str( len( response ) ) ) ] )

    return [ str( response ) ]

beach = Beach( sys.argv[ 1 ], realm = 'hcp' )
vHandle = beach.getActorHandle( 'c2/beacon', nRetries = 3, timeout = 30, ident = 'http/5bc10821-2d3f-413a-81ee-30759b9f863b' )

server = wsgi.WSGIServer( ( '', 80 ), handle_beacon, spawn = 100 )

server.serve_forever()