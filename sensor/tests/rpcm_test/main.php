<?php

chdir( dirname( __FILE__ ) );
require_once( '../../lib/php_libs/rpcm/librpcm.php' );

require_once( '../../lib/simpletest/autorun.php' );

class rpcm_testing extends UnitTestCase
{
    function test_create()
    {
        $seq = new rpcm_SEQUENCE();
        
        $ru8 = new rpcm_RU8( 42, 666 );
        $ru16 = new rpcm_RU16( 42, 666 );
        $ru32 = new rpcm_RU32( 42, 666 );
        $ru64 = new rpcm_RU64( 42, 666 );
        $stringa = new rpcm_STRINGA( 'hello', 666 );
        $stringw = new rpcm_STRINGW( 'you', 666 );
        
        $this->assertTrue( $seq->isValid() );
        $this->assertTrue( $ru8->isValid() );
        $this->assertTrue( $ru16->isValid() );
        $this->assertTrue( $ru32->isValid() );
        $this->assertTrue( $ru64->isValid() );
        $this->assertTrue( $stringa->isValid() );
        $this->assertTrue( $stringw->isValid() );
        
        $this->assertEqual( $ru8->getValue(), 42 );
        $this->assertEqual( $ru16->getValue(), 42 );
        $this->assertEqual( $ru32->getValue(), 42 );
        $arr = $ru64->getValue();
        $this->assertEqual( $arr[ 'low' ], 42 );
        $this->assertEqual( $arr[ 'high' ], 0 );
        $this->assertEqual( $stringa->getValue(), 'hello' );
        $this->assertEqual( $stringw->getValue(), 'you' );
        
        $ru8 = new rpcm_RU8( 0xFFF );
        $ru16 = new rpcm_RU16( 0xFFFFF );
        $ru32 = new rpcm_RU32( 0xFFFFFFFFFF );
        
        $this->assertFalse( $ru8->isValid() );
        $this->assertFalse( $ru16->isValid() );
        $this->assertFalse( $ru32->isValid() );
    }
    
    
    function test_add()
    {
        $seq = new rpcm_SEQUENCE();
        $list = new rpcm_LIST();
        
        $ru8 = new rpcm_RU8( 42 );
        $ru16 = new rpcm_RU16( 42 );
        $ru32 = new rpcm_RU32( 42 );
        $ru64 = new rpcm_RU64( 42 );
        $stringa = new rpcm_STRINGA( 'hello' );
        $stringw = new rpcm_STRINGW( 'you' );
        
        $lstr1 = new rpcm_STRINGA( 'str1', 666 );
        $lstr2 = new rpcm_STRINGA( 'str2', 666 );
        $lstr3 = new rpcm_STRINGA( 'str3', 666 );
        $lstrnot = new rpcm_STRINGA( 'strnot', 777 );
        $lnot = new rpcm_RU32( 32, 666 );
        
        $this->assertTrue( $seq->addElement( $ru8, 1 ) );
        $this->assertTrue( $seq->addElement( $ru16, 2 ) );
        $this->assertTrue( $seq->addElement( $ru32, 3 ) );
        $this->assertTrue( $seq->addElement( $ru64, 4 ) );
        $this->assertTrue( $seq->addElement( $stringa, 5 ) );
        $this->assertTrue( $seq->addElement( $stringw, 6 ) );
        
        $this->assertEqual( 6, $seq->getNumElements() );
        
        $this->assertTrue( $list->addElement( $lstr1 ) );
        $this->assertTrue( $list->addElement( $lstr2 ) );
        $this->assertTrue( $list->addElement( $lstr3 ) );
        $this->assertFalse( $list->addElement( $lstrnot ) );
        $this->assertFalse( $list->addElement( $lnot ) );
        
        $this->assertEqual( 3, $list->getNumElements() );
        
        $this->assertTrue( $seq->addElement( $list, 7 ) );
        
        $this->assertEqual( 7, $seq->getNumElements() );
        
        $newlist = $seq->getElement( 7 );
        $this->assertTrue( $newlist->isValid() );
        $this->assertEqual( 3, $newlist->getNumElements() );
        
        $this->assertFalse( $seq->getElement( 8 ) );
        $this->assertFalse( $seq->getElement( 7, RPCM_STRINGA ) );
    }
    
    function test_serialise()
    {
        $seq = new rpcm_SEQUENCE();
        $list = new rpcm_LIST();
        
        $ru8 = new rpcm_RU8( 42 );
        $ru16 = new rpcm_RU16( 42 );
        $ru32 = new rpcm_RU32( 42 );
        $ru64 = new rpcm_RU64( 42 );
        $stringa = new rpcm_STRINGA( 'hello' );
        $stringw = new rpcm_STRINGW( 'you' );
        
        $lstr1 = new rpcm_STRINGA( 'str1', 666 );
        $lstr2 = new rpcm_STRINGA( 'str2', 666 );
        $lstr3 = new rpcm_STRINGA( 'str3', 666 );
        $lstrnot = new rpcm_STRINGA( 'strnot', 777 );
        $lnot = new rpcm_RU32( 32, 666 );
        
        $this->assertTrue( $seq->addElement( $ru8, 1 ) );
        $this->assertTrue( $seq->addElement( $ru16, 2 ) );
        $this->assertTrue( $seq->addElement( $ru32, 3 ) );
        $this->assertTrue( $seq->addElement( $ru64, 4 ) );
        $this->assertTrue( $seq->addElement( $stringa, 5 ) );
        $this->assertTrue( $seq->addElement( $stringw, 6 ) );
        
        $this->assertTrue( $list->addElement( $lstr1 ) );
        $this->assertTrue( $list->addElement( $lstr2 ) );
        $this->assertTrue( $list->addElement( $lstr3 ) );
        
        $this->assertTrue( $seq->addElement( $list, 7 ) );
        
        $this->assertTrue( $seq->serialise( $buffer ) );
        
        $newseq = new rpcm_SEQUENCE();
        $this->assertTrue( $newseq->deserialise( $buffer ) );
        
        $this->assertEqual( $newseq->getNumElements(), $seq->getNumElements() );
    }
    /*
    function test_platformCompat()
    {
        $c_serial = file_get_contents( './rpcm_test_seq' );
        $this->assertTrue( $c_serial );
        
        $seq = new rpcm_SEQUENCE();
        $this->assertTrue( $seq->deserialise( $c_serial ) );
        //var_dump( $seq );
    }
    */
    
    function test_xml()
    {
        require_once( '../../lib/php_libs/rpcm/xml.php' );
        
        $list = new rpcm_LIST();
        $seq1 = new rpcm_SEQUENCE();
        $seq2 = new rpcm_SEQUENCE();
        
        $seq1->addElement( new rpcm_RU64( 666 ), RP_TAGS_MAC_ADDRESS );
        $seq1->addElement( new rpcm_RU8( 42 ), RP_TAGS_TIMESTAMP );
        $seq1->addElement( new rpcm_STRINGW( "hello" ), RP_TAGS_RAPTD_STATUS );
        $seq1->addElement( new rpcm_TIMESTAMP( "june 1st 2011" ), RP_TAGS_HASH );
        $seq1->addElement( new rpcm_BUFFER( 'this is a test buffer...' ), RP_TAGS_HCP_ID );
        
        $seq2->addElement( new rpcm_POINTER_32( 666 ), RP_TAGS_MAC_ADDRESS );
        $seq2->addElement( new rpcm_RU8( 42 ), RP_TAGS_TIMESTAMP );
        $seq2->addElement( new rpcm_STRINGA( "?hello!" ), RP_TAGS_RAPTD_STATUS );
        $seq2->addElement( new rpcm_TIMESTAMP( "june 1st 2011" ), RP_TAGS_HASH );
        
        $this->assertTrue( $list->addElement( $seq1, RP_TAGS_HOST_NAME ) );
        $this->assertTrue( $list->addElement( $seq2, RP_TAGS_HOST_NAME ) );
        $list->setTag( RP_TAGS_IP_ADDRESS );
        
        $this->assertTrue( $list->isValid() );
        $xml = RpcmXml::toXml( $list );
        
        $this->assertTrue( 0 != strlen( $xml ) );
        
        $newList = RpcmXml::toElem( $xml );
        
        $this->assertEqual( count( $list->getElements() ), count( $newList->getElements() ) );
    }
    
    /*
    function test_xmlToSeq()
    {
        rpcm_element::setTraceError( TRUE );
        $this->assertTrue( $seq = RpcmXml::toElem( file_get_contents( '/home/server/testbundle.xml' ) ) );
        $this->assertTrue( $seq->serialise( $buff ) );
        $newseq = new rpcm_SEQUENCE();
        $this->assertTrue( $newseq->deserialise( $buff ) );
    }
    */
    
    function test_xpaths()
    {
        $list = new rpcm_LIST();
        $seq1 = new rpcm_SEQUENCE();
        $seq2 = new rpcm_SEQUENCE();
        
        $seq1->addElement( new rpcm_RU64( 666 ), RP_TAGS_MAC_ADDRESS );
        $seq1->addElement( new rpcm_RU8( 42 ), RP_TAGS_TIMESTAMP );
        $seq1->addElement( new rpcm_STRINGW( "hello" ), RP_TAGS_RAPTD_STATUS );
        $seq1->addElement( new rpcm_TIMESTAMP( "june 1st 2011" ), RP_TAGS_HASH );
        $seq1->addElement( new rpcm_BUFFER( 'this is a test buffer...' ), RP_TAGS_HCP_ID );
        
        $seq2->addElement( new rpcm_POINTER_32( 666 ), RP_TAGS_MAC_ADDRESS );
        $seq2->addElement( new rpcm_RU8( 42 ), RP_TAGS_TIMESTAMP );
        $seq2->addElement( new rpcm_STRINGA( "?hello!" ), RP_TAGS_RAPTD_STATUS );
        $seq2->addElement( new rpcm_TIMESTAMP( "june 1st 2011" ), RP_TAGS_HASH );
        
        $this->assertTrue( $list->addElement( $seq1, RP_TAGS_HOST_NAME ) );
        $this->assertTrue( $list->addElement( $seq2, RP_TAGS_HOST_NAME ) );
        $list->setTag( RP_TAGS_IP_ADDRESS );
        
        $this->assertTrue( $list->isValid() );
        
        $elem = $list->getXPath( 'RP_TAGS_HOST_NAME' );
        $this->assertTrue( 2 == count( $elem ) );
        
        $elem = $list->getXPath( 'RP_TAGS_HOST_NAME/RP_TAGS_MAC_ADDRESS' );
        $this->assertTrue( 2 == count( $elem ) );
        
        $dummy = new rpcm_RU32( 444 );
        $this->assertTrue( $list->setXPath( 'RP_TAGS_HOST_NAME', $dummy, RP_TAGS_HOST_NAME ) );
        
        $elem = $list->getXPath( 'RP_TAGS_HOST_NAME/RP_TAGS_HOST_NAME' );
        $this->assertTrue( 2 == count( $elem ) );
    }
    
}


?>