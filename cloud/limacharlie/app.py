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

import os
import sys

from beach.beach_api import Beach

import traceback
import web
import datetime
import time
import json
from functools import wraps


###############################################################################
# CUSTOM EXCEPTIONS
###############################################################################


###############################################################################
# REFERENCE ELEMENTS
###############################################################################


###############################################################################
# CORE HELPER FUNCTIONS
###############################################################################
def tsToTime( ts ):
    return datetime.datetime.fromtimestamp( int( ts ) ).strftime( '%Y-%m-%d %H:%M:%S' )

###############################################################################
# PAGE DECORATORS
###############################################################################
def jsonApi( f ):
    ''' Decorator to basic exception handling on function. '''
    @wraps( f )
    def wrapped( *args, **kwargs ):
        web.header( 'Content-Type', 'application/json' )
        r = f( *args, **kwargs )
        try:
            return json.dumps( r )
        except:
            return json.dumps( { 'error' : str( r ) } )
    return wrapped

###############################################################################
# PAGES
###############################################################################
class Index:
    def GET( self ):
        return render.index()

class Dashboard:
    def GET( self ):

        sensors = model.request( 'list_sensors', {} )

        if not sensors.isSuccess:
            return render.error( str( sensors ) )

        return render.dashboard( sensors = sensors.data )

class Sensor:
    def GET( self ):
        params = web.input( sensor_id = None, before = None, after = None )

        if params.sensor_id is None:
            return render.error( 'sensor_id required' )

        info = model.request( 'get_sensor_info', { 'id_or_host' : params.sensor_id } )

        if not info.isSuccess:
            return render.error( str( info ) )

        if 0 == len( info.data ):
            return render.error( 'Sensor not found' )

        before = None
        after = None

        if '' != params.before:
            before = params.before
        if '' != params.after:
            after = params.after

        return render.sensor( info.data[ 'id' ], before, after )

class SensorState:
    @jsonApi
    def GET( self ):
        params = web.input( sensor_id = None )

        if params.sensor_id is None:
            raise web.HTTPError( '400 Bad Request: sensor id required' )

        info = model.request( 'get_sensor_info', { 'id_or_host' : params.sensor_id } )

        if not info.isSuccess:
            raise web.HTTPError( '503 Service Unavailable: %s' % str( info ) )

        if 0 == len( info.data ):
            raise web.HTTPError( '204 No Content: sensor not found' )

        return info.data

class Timeline:
    @jsonApi
    def GET( self ):
        params = web.input( sensor_id = None, after = None, before = None )

        if params.sensor_id is None:
            raise web.HTTPError( '400 Bad Request: sensor id required' )

        if params.after is None or '' == params.after:
            raise web.HTTPError( '400 Bad Request: need start time' )

        start_time = int( params.after )

        if 0 == start_time:
            start_time = int( time.time() )

        req = { 'id' : params.sensor_id,
                'is_include_content' : True,
                'after' : start_time,
                'mas_size' : 512 }

        if params.before is not None and '' != params.before:
            req[ 'before' ] = int( params.before )

        info = model.request( 'get_timeline', req )

        if not info.isSuccess:
            return render.error( str( info ) )

        if 0 == int( params.after ):
            info.data[ 'new_start' ] = start_time

        return info.data

class ObjSearch:
    def GET( self ):
        params = web.input( objname = None )

        if params.objname is None:
            return render.error( 'Must specify an object name' )

        objects = model.request( 'get_obj_list', { 'name' : params.objname } )

        if not objects.isSuccess:
            return render.error( str( objects ) )

        return render.objlist( objects.data[ 'objects' ], None )

class ObjViewer:
    def GET( self ):
        params = web.input( sensor_id = None, id = None )

        if params.id is None:
            return render.error( 'need to supply an object id' )

        req = { 'id' : params.id }

        if params.sensor_id is not None:
            req[ 'host' ] = params.sensor_id

        info = model.request( 'get_obj_view', req )

        if not info.isSuccess:
            return render.error( str( info ) )

        return render.obj( info.data, params.sensor_id )

class LastEvents:
    @jsonApi
    def GET( self ):
        params = web.input( sensor_id = None )

        if params.sensor_id is None:
            raise web.HTTPError( '400 Bad Request: sensor id required' )

        info = model.request( 'get_lastevents', { 'id' : params.sensor_id } )

        if not info.isSuccess:
            raise web.HTTPError( '503 Service Unavailable : %s' % str( info ) )

        return info.data.get( 'events', [] )

class EventView:
    def GET( self ):
        params = web.input( id = None )

        if params.id is None:
            return render.error( 'need to supply an event id' )

        info = model.request( 'get_event', { 'id' : params.id } )

        if not info.isSuccess:
            return render.error( str( info ) )

        return render.event( info.data.get( 'event', {} ) )

class HostObjects:
    def GET( self ):
        params = web.input( sensor_id = None, otype = None )

        if params.sensor_id is None:
            return render.error( 'need to supply a sensor id' )

        req = { 'host' : params.sensor_id }

        if params.otype is not None:
            req[ 'type' ] = params.otype

        objects = model.request( 'get_obj_list', req )

        return render.objlist( objects.data[ 'objects' ], params.sensor_id )

###############################################################################
# BOILER PLATE
###############################################################################
os.chdir( os.path.dirname( os.path.abspath( __file__ ) ) )

urls = ( r'/', 'Index',
         r'/dashboard', 'Dashboard',
         r'/sensor', 'Sensor',
         r'/search', 'Search',
         r'/sensor_state', 'SensorState',
         r'/timeline', 'Timeline',
         r'/objsearch', 'ObjSearch',
         r'/obj', 'ObjViewer',
         r'/lastevents', 'LastEvents',
         r'/event', 'EventView',
         r'/hostobjects', 'HostObjects' )

web.config.debug = False
app = web.application( urls, globals() )

render = web.template.render( 'templates', base = 'base', globals = { 'json' : json, 'tsToTime' : tsToTime } )

if len( sys.argv ) < 2:
    print( "Usage: python app.py beach_config [listen_port]" )
    sys.exit()

beach = Beach( sys.argv[ 1 ], realm = 'hcp' )
del( sys.argv[ 1 ] )
model = beach.getActorHandle( 'models', nRetries = 3, timeout = 30, ident = 'lc/0bf01f7e-62bd-4cc4-9fec-4c52e82eb903' )

app.run()