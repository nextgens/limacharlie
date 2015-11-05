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

import struct
import socket
import traceback
import json



def isTypeIn( elem, types ):
    return elem.__class__.__name__ in types



class rSequence( dict ):
    
    def addInt8( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_RU8 }
        return self
    
    def addInt16( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_RU16 }
        return self
    
    def addInt32( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_RU32 }
        return self
    
    def addInt64( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_RU64 }
        return self

    def addTimestamp( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_TIMESTAMP }
        return self
    
    def addStringA( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_STRINGA }
        return self
    
    def addStringW( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_STRINGW }
        return self
    
    def addBuffer( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_BUFFER }
        return self
    
    def addIpv4( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_IPV4 }
        return self
    
    def addIpv6( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_IPV6 }
        return self
    
    def addPtr32( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_POINTER_32 }
        return self
    
    def addPtr64( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_POINTER_64 }
        return self
    
    def addTimedelta( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_TIMEDELTA }
        return self
    
    def addSequence( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_SEQUENCE }
        return self
    
    def addList( self, tag, value ):
        self[ tag ] = { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_LIST }
        return self

class rList( list ):
    
    def addInt8( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_RU8 } )
        return self
    
    def addInt16( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_RU16 } )
        return self
    
    def addInt32( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_RU32 } )
        return self
    
    def addInt64( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_RU64 } )
        return self

    def addTimestamp( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_TIMESTAMP } )
        return self
    
    def addStringA( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_STRINGA } )
        return self
    
    def addStringW( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_STRINGW } )
        return self
    
    def addBuffer( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_BUFFER } )
        return self
    
    def addIpv4( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_IPV4 } )
        return self
    
    def addIpv6( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_IPV6 } )
        return self
    
    def addPtr32( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_POINTER_32 } )
        return self
    
    def addPtr64( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_POINTER_64 } )
        return self
    
    def addTimedelta( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_TIMEDELTA } )
        return self
    
    def addSequence( self, tag, value ):
        self.append( { 'tag' : tag, 'value' : value, 'type' : rpcm.RPCM_SEQUENCE } )
        return self

class rpcm( object ):
    
    # Data type constants
    #===========================================================================
    RPCM_INVALID_TAG = 0
    RPCM_INVALID_TYPE = 0
    RPCM_RU8 = 0x01
    RPCM_RU16 = 0x02
    RPCM_RU32 = 0x03
    RPCM_RU64 = 0x04
    RPCM_STRINGA = 0x05
    RPCM_STRINGW = 0x06
    RPCM_BUFFER = 0x07
    RPCM_TIMESTAMP = 0x08
    RPCM_IPV4 = 0x09
    RPCM_IPV6 = 0x0A
    RPCM_POINTER_32 = 0x0B
    RPCM_POINTER_64 = 0x0C
    RPCM_TIMEDELTA = 0x0D
    RPCM_COMPLEX_TYPES = 0x80
    RPCM_SEQUENCE = 0x81
    RPCM_LIST = 0x82
    
    def __init__( self, isDebug = False, isDetailedDeserialize = False, isHumanReadable = False ):
        self._data = None
        self._offset = 0
        self._symbols = None
        self._typeSymbols = None
        self._isTracing = False
        
        self._isDebug = isDebug
        self._isDetailedDeserialize = isDetailedDeserialize
        self._isHumanReadable = isHumanReadable
        
        self._typeParsers = { self.RPCM_RU8 : self._parse_numbers,
                              self.RPCM_RU16 : self._parse_numbers,
                              self.RPCM_RU32 : self._parse_numbers,
                              self.RPCM_RU64 : self._parse_numbers,
                              self.RPCM_TIMESTAMP : self._parse_numbers,
                              self.RPCM_STRINGA : self._parse_string,
                              self.RPCM_STRINGW : self._parse_string,
                              self.RPCM_BUFFER : self._parse_string,
                              self.RPCM_IPV4 : self._parse_ipv4,
                              self.RPCM_IPV6 : self._parse_ipv6,
                              self.RPCM_POINTER_32 : self._parse_numbers,
                              self.RPCM_POINTER_64 : self._parse_numbers,
                              self.RPCM_TIMEDELTA : self._parse_numbers,
                              self.RPCM_SEQUENCE : self._parse_sequence,
                              self.RPCM_LIST : self._parse_list }
        
        self._typePackers = { self.RPCM_RU8 : self._pack_numbers,
                              self.RPCM_RU16 : self._pack_numbers,
                              self.RPCM_RU32 : self._pack_numbers,
                              self.RPCM_RU64 : self._pack_numbers,
                              self.RPCM_TIMESTAMP : self._pack_numbers,
                              self.RPCM_STRINGA : self._pack_string,
                              self.RPCM_STRINGW : self._pack_string,
                              self.RPCM_BUFFER : self._pack_string,
                              self.RPCM_IPV4 : self._pack_ipv4,
                              self.RPCM_IPV6 : self._pack_ipv6,
                              self.RPCM_POINTER_32 : self._pack_numbers,
                              self.RPCM_POINTER_64 : self._pack_numbers,
                              self.RPCM_TIMEDELTA : self._pack_numbers,
                              self.RPCM_SEQUENCE : self._pack_sequence,
                              self.RPCM_LIST : self._pack_list }
        
        if isHumanReadable:
            self._typeSymbols = { self.RPCM_RU8 : 'int_8',
                                 self.RPCM_RU16 : 'int_16',
                                 self.RPCM_RU32 : 'int_32',
                                 self.RPCM_RU64 : 'int_64',
                                 self.RPCM_TIMESTAMP : 'timestamp',
                                 self.RPCM_STRINGA : 'string_a',
                                 self.RPCM_STRINGW : 'string_w',
                                 self.RPCM_BUFFER : 'buffer',
                                 self.RPCM_IPV4 : 'ipv4',
                                 self.RPCM_IPV6 : 'ipv6',
                                 self.RPCM_POINTER_32 : 'pointer_32',
                                 self.RPCM_POINTER_64 : 'pointer_64',
                                 self.RPCM_TIMEDELTA : 'timedelta',
                                 self.RPCM_SEQUENCE : 'sequence',
                                 self.RPCM_LIST : 'list',
                                 'int_8' : self.RPCM_RU8,
                                 'int_16' : self.RPCM_RU16,
                                 'int_32' : self.RPCM_RU32,
                                 'int_64' : self.RPCM_RU64,
                                 'timestamp' : self.RPCM_TIMESTAMP,
                                 'string_a' : self.RPCM_STRINGA,
                                 'string_w' : self.RPCM_STRINGW,
                                 'buffer' : self.RPCM_BUFFER,
                                 'ipv4' : self.RPCM_IPV4,
                                 'ipv6' : self.RPCM_IPV6,
                                 'pointer_32' : self.RPCM_POINTER_32,
                                 'pointer_64' : self.RPCM_POINTER_64,
                                 'timedelta' : self.RPCM_TIMEDELTA,
                                 'sequence' : self.RPCM_SEQUENCE,
                                 'list' : self.RPCM_LIST }
    
    def _printDebug( self, msg ):
        if self._isDebug:
            self._isDebug( msg )
    
    def _printTrace( self, msg ):
        if self._isTracing:
            print( msg )
    
    def _printBuffer( self, buff, start, size ):
        out = ''
        end = start + size
        if end > len( buff ):
            end = len( buff )
        for i in range( start, start + size ):
            out += '%s ' % hex( ord( buff[ i ] ) )
        
        return out
    
    def _consume( self, frm ):
        tup = None
        required = struct.calcsize( frm )
        
        try:
            tup = struct.unpack_from( frm, self._data, self._offset )
        except:
            self._printDebug( traceback.format_exc() )
            tup = None
        
        if None != tup:
            self._offset += required
            
            if 1 == len( tup ):
                tup = tup[ 0 ]
            
        return tup
    
    def _deserialise_element( self, expected_tag = None, expected_typ = None ):
        result = None
        
        headers = self._consume( '>IB' )
        if None != headers:
            tag = str( headers[ 0 ] )
            typ = headers[ 1 ]
            
            self._printTrace( 'parsed element header: %s / %s' % ( expected_tag, expected_typ ) )
            
            if( expected_tag == None or str( tag  )== str( expected_tag ) ) and ( expected_typ == None or str( typ ) == str( expected_typ ) ):
                if typ in self._typeParsers:
                    if self._isHumanReadable and None != self._symbols:
                        if str( tag ) in self._symbols:
                            tag = self._symbols[ str( tag ) ]
                            self._printTrace( 'tag lookup yielded: %s' % tag )
                        else:
                            self._printTrace( 'tag lookup not found' )
                            
                    try:
                        value = self._typeParsers[ typ ]( typ )
                        
                        self._printTrace( 'element parsed yielded value: %s' % value )
                        
                        if None != value and self._isDetailedDeserialize:
                            if self._isHumanReadable and typ in self._typeSymbols:
                                strtyp = self._typeSymbols[ typ ]
                            else:
                                strtyp = typ
                            value = { 'type' : strtyp, 'tag' : tag, 'value' : value }
                        
                        result = { str( tag ) : value }
                    except:
                        self._printDebug( traceback.format_exc() )
                        result = None
                else:
                    self._printDebug( 'unknown type: %d / %d' % ( int( tag ), typ ) )
            else:
                self._printDebug( 'expected %s/%s but got %s/%s' % ( expected_tag, expected_typ, tag, typ ) )
        else:
            self._printDebug( 'not enough data: %d' % len( self._data ) )
        
        return result
    
    def _serialise_element( self, elem ):
        result = None
        
        if 'tag' in elem and 'type'  in elem and 'value' in elem:
            tag = elem[ 'tag' ]
            typ = elem[ 'type' ]
                
            if self._isHumanReadable and None != self._symbols:
                if type( tag ) is not int and not tag.isdigit() and str( tag ) in self._symbols:
                    tag = self._symbols[ str( tag ) ]
                if type( typ ) is not int and not typ.isdigit() and typ in self._typeSymbols:
                    typ = self._typeSymbols[ typ ]
                
            result = struct.pack( '>IB', int( tag ), typ )
            
            if typ in self._typePackers:
                tmpPacked = self._typePackers[ typ ]( typ, elem[ 'value' ] )
                if tmpPacked is not None:
                    result += tmpPacked
                else:
                    self._printDebug( 'error packing %s with value %s' % ( typ, elem[ 'value' ] ) )
                    result = None
            else:
                self._printDebug( 'unknown type: %d / %d' % ( int( elem[ 'tag' ] ), elem[ 'type' ] ) )
                result = None
        else:
            self._printDebug( 'missing information: %s' % json.dumps( elem ) )
        
        return result
    
    def _json_to_rpcm( self, j ):
        res = None
        
        if type( j ) is list:
            res = rList()
            
            for elem in j:
                res.append( self._json_to_rpcm( elem ) )
                
        elif type( j ) is dict and ( 'tag' in j and 'type' in j and 'value' in j ):
            res = j
        elif type( j ) is dict:
            res = rSequence()
            
            for tag, val in j.iteritems():
                res[ tag ] = self._json_to_rpcm( val )
        else:
            self._printDebug( 'unexpected structure in json to rpcm: %s' % str( j ) )
        
        return res
    
    
    # Data type parsing
    #===========================================================================
    def _parse_numbers( self, typ ):
        number = None
        if self.RPCM_RU8 == typ:
            number = self._consume( '>B' )
        elif self.RPCM_RU16 == typ:
            number = self._consume( '>H' )
        elif self.RPCM_RU32 == typ or self.RPCM_POINTER_32  == typ:
            number = self._consume( '>I' )
        elif self.RPCM_RU64 == typ or self.RPCM_TIMESTAMP == typ or self.RPCM_POINTER_64  == typ or self.RPCM_TIMEDELTA == typ:
            number = self._consume( '>Q' )
        
        return number
    
    def _parse_string( self, typ ):
        string = None
        
        size = self._consume( '>I' )
        if None != size:
            
            string = self._consume( '%ds' % size )
            if None != string:
                try:
                    if self.RPCM_STRINGA == typ:
                        if 0 == ord( string[ -1 ] ):
                            string = string[ : -1 ]
                        string = str( string )
                    elif self.RPCM_STRINGW == typ:
                        if 0 == ord( string[ -1 ] ):
                            string = string[ : -1 ]
                        string = string.decode( 'utf-8' )
                    elif self.RPCM_BUFFER == typ:
                        string = str( string )
                except:
                    string = None
                    self._printDebug( traceback.format_exc() )
        
        return string
    
    def _parse_ipv4( self, typ ):
        ip = None
        
        ip = self._consume( '>I' )
        
        if None != ip:
            ip = socket.inet_ntoa( struct.pack( '<I', ip ) )
        
        return ip
    
    def _parse_ipv6( self, typ ):
        ip = None
        
        ip = self._consume( '16s' )
        
        return ip
    
    def _parse_set( self, typ, expected_tag = None, expected_typ = None ):
        s = None
        
        isList = True if( self.RPCM_LIST == typ ) else False
        
        nElements = self._consume( '>I' )
        
        self._printTrace( 'parsing set, expecting %s elements' % ( nElements, ) )
        
        if None != nElements:
            if isList:
                s = rList()
            else:
                s = rSequence()
            
            for n in range( 0, nElements ):
                self._printTrace( 'parsing element in set' )
                
                tmpElem = self._deserialise_element( expected_tag, expected_typ )
                
                if None != tmpElem:
                    if isList:
                        self._printTrace( 'adding new element in list' )
                        s.append( tmpElem.values()[ 0 ] )
                    else:
                        self._printTrace( 'adding new element to seq' )
                        s = rSequence( s.items() + tmpElem.items() )
                else:
                    s = None
                    self._printTrace( 'no element could be parsed' )
                    break
        
        return s
    
    def _parse_sequence( self, typ ):
        return self._parse_set( self.RPCM_SEQUENCE )
    
    def _parse_list( self, typ ):
        l = None
        
        headers = self._consume( '>IB' )
        if None != headers:
            expected_tag = headers[ 0 ]
            expected_typ = headers[ 1 ]
            
            self._printTrace( 'parsing list, expecting: %s / %s' % ( expected_tag, expected_typ ) )
            
            l = self._parse_set( self.RPCM_LIST, expected_tag, expected_typ )
        
        return l
    
    
    
    
    
    
    def _pack_numbers( self, typ, value ):
        number = None
        if self.RPCM_RU8 == typ:
            number = struct.pack( '>B', int( value ) )
        elif self.RPCM_RU16 == typ:
            number = struct.pack( '>H', int( value ) )
        elif self.RPCM_RU32 == typ or self.RPCM_POINTER_32  == typ:
            number = struct.pack( '>I', int( value ) )
        elif self.RPCM_RU64 == typ or self.RPCM_TIMESTAMP == typ or self.RPCM_POINTER_64  == typ or self.RPCM_TIMEDELTA == typ:
            number = struct.pack( '>Q', int( value ) )
        
        return number
    
    def _pack_string( self, typ, value ):
        string = None
        
        try:
            if self.RPCM_STRINGA == typ:
                string = struct.pack( '>I%dsB' % len( value ), len( value ) + 1, value, 0 )
            elif self.RPCM_STRINGW == typ:
                value = value.encode( 'utf-8' )
                string = struct.pack( '>I%dsB' % len( value ), len( value ) + 1, value, 0 )
            elif self.RPCM_BUFFER == typ:
                string = struct.pack( '>I%ds' % len( value ), len( value ), value )
        except:
            string = None
            self._printDebug( traceback.format_exc() )
        
        return string
    
    def _pack_ipv4( self, typ, value ):
        ip = None
        
        try:
            ip = struct.pack( '<I', int( socket.inet_aton( value ).encode( 'hex' ), 16 ) )
        except:
            self._printDebug( traceback.format_exc() )
        
        return ip
    
    def _pack_ipv6( self, typ, value ):
        ip = None
        
        ip = struct.pack( '16s', value )
        
        return ip
    
    def _pack_set( self, typ, value ):
        s = None
        
        isList = True if( self.RPCM_LIST == typ ) else False
        
        try:
            s = struct.pack( '>I', len( value ) )
            
            if isList:
                values = value
            else:
                values = value.values()
                
            for elem in values:
                s += self._serialise_element( elem )
        except:
            self._printDebug( traceback.format_exc() )
            s = None
        
        return s
    
    def _pack_sequence( self, typ, value ):
        return self._pack_set( typ, value )
    
    def _pack_list( self, typ, value ):
        l = None
        
        expected_tag = self.RPCM_INVALID_TAG
        expected_type = self.RPCM_INVALID_TYPE
        if 0 != len( value ):
            expected_tag = value[ 0 ][ 'tag' ]
            expected_type = value[ 0 ][ 'type' ]
            if self._isHumanReadable:
                if type( expected_type ) is not int and not expected_type.isdigit() and expected_type in self._typeSymbols:
                    expected_type = self._typeSymbols[ expected_type ]
                if type( expected_tag ) is not int and not expected_tag.isdigit() and None != self._symbols and expected_tag in self._symbols:
                    expected_tag = self._symbols[ str( expected_tag ) ]
        l = struct.pack( '>IB', int( expected_tag ), expected_type )
        
        l += self._pack_set( typ, value )
        
        return l
    
    
    # Public interface
    #===========================================================================
    def setBuffer( self, buff ):
        self._data = buff
        self._offset = 0
    
    def dataInBuffer( self ):
        return self._data[ self._offset : ] if self._data is not None else ''
        
    def deserialise( self, isList = False, isTracing = False ):
        self._isTracing = isTracing
        
        # The roots are always a Sequence
        if isList:
            self._printTrace( 'deserialise list' )
            return self._parse_list( self.RPCM_LIST )
        else:
            self._printTrace( 'deserialise seq' )
            return self._parse_sequence( self.RPCM_SEQUENCE )
        
        self._isTracing = False
    
    def serialise( self, seq ):
        self._data = ''
        self._offset = 0

        if isTypeIn( seq, ( 'list', 'rList' ) ):
            return self._pack_list( self.RPCM_LIST, seq )
        else:
            return self._pack_sequence( self.RPCM_SEQUENCE, seq )


    def loadSymbols( self, symbolsDict ):
        self._symbols = symbolsDict

    def loadJson( self, jStr ):
        if type( jStr ) is str or type( jStr ) is unicode:
            j = json.loads( jStr )
        else:
            j = jStr
        
        return self._json_to_rpcm( j )
    
    def convertTag( self, tag ):
        converted = None
        
        tag = str( tag )
        if self._symbols and tag in self._symbols:
            converted = self._symbols[ tag ]
        
        return converted
        







#===============================================================================
#   Some testing routines
#===============================================================================
if '__main__' == __name__:
    import json
    import os
    os.chdir( os.path.dirname( os.path.abspath( __file__ ) ) )
    
    r = rpcm( isDebug = True, isDetailedDeserialize = True, isHumanReadable = True )
    #f = open( './tags.json', 'r' )
    #r.loadSymbols( json.loads( f.read() ) )
    r.loadSymbols( { "666" : "devil", "devil" : 666 } )
    #f.close()
    
    print( "TESTING FROM C FILE" )
    f = open( './rpcm_test_seq', 'r' )
    b = f.read()
    f.close()
    
    print( "Packed len: %d" % len( b ) )
    r.setBuffer( b )
    a = r.deserialise()
    
    
    print( "TESTING SERIALISING" )
    c = r.serialise( a )
    print( "Packed len: %d" % len( c ) )
    
    
    print( "TESTING DESERIALISING" )
    r.setBuffer( c )
    d = r.deserialise()
    
    print( "TESTING SERIALISING 2" )
    e = r.serialise( d )
    print( "Packed len: %d" % len( e ) )
    
    
    print( "COMPARING:" )
    print( json.dumps( a, indent = 4 ) )
    print( "-----------------------------------" )
    print( json.dumps( d, indent = 4 ) )
    
    import binascii
    print( binascii.b2a_hex( b ) )
    print( "-----------------------------------" )
    print( binascii.b2a_hex( c ) )
    print( "-----------------------------------" )
    print( binascii.b2a_hex( e ) )
    
    
    print( "\n\n\n\nSimple Mode:\n\n" )
    r = rpcm( isDebug = True, isDetailedDeserialize = False, isHumanReadable = True )
    #f = open( './tags.json', 'r' )
    #r.loadSymbols( json.loads( f.read() ) )
    r.loadSymbols( { "666" : "devil", "devil" : 666 } )
    #f.close()
    
    print( "TESTING FROM C FILE" )
    f = open( './rpcm_test_seq', 'r' )
    b = f.read()
    f.close()
    
    print( "Packed len: %d" % len( b ) )
    a = r.setBuffer( b )
    a = r.deserialise()
    print( json.dumps( a, indent = 4 ) )
    print( a._( '88/77/66' ) )
    print( a._( '88/*/42' ) )
    print( a._( '88/?/24' ) )
    
    a[ '666' ] = r.loadJson( [ { 'a' : 'yes', 'b' : 'no' }, { 'a' : 'maybe', 'b' : 'forsure' } ] )
    n = r.loadJson( a )
    print( a._( '666/b' ) )

