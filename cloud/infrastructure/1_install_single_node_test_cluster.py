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

if os.geteuid() != 0:
    print( 'Please run me as root to setup this test, but don\'t ever do that in production!' )
    sys.exit(-1)

root = os.path.join( os.path.abspath( os.path.dirname( __file__ ) ), '..', '..' )

def printStep( step, *ret ):
    msg = '''
===============
Step: %s
Return Values: %s
===============

''' % ( step, str( ret ) )
    print( msg )


printStep( 'Updating repo and upgrading existing components.',
    os.system( 'apt-get update -y' ),
    os.system( 'apt-get upgrade -y' ) )

printStep( 'Installing some basic packages required for Beach (mainly).',
    os.system( 'apt-get install python-pip python-dev debconf-utils python-m2crypto python-pexpect python-mysqldb -y' ) )

printStep( 'Installing Beach.',
    os.system( 'pip install beach' ) )

printStep( 'Installing JRE for Cassandra (the hcp-scale-db)',
    os.system( 'apt-get install default-jre -y' ) )

printStep( 'Installing Cassandra.',
    os.system( 'echo "deb http://debian.datastax.com/community stable main" | sudo tee -a /etc/apt/sources.list.d/cassandra.sources.list' ),
    os.system( 'curl -L http://debian.datastax.com/debian/repo_key | sudo apt-key add -' ),
    os.system( 'apt-get update -y' ),
    os.system( 'apt-get install cassandra -y' ) )

printStep( 'Installing MySql server (hcp-state-db).',
    os.system( 'echo mysql-server mysql-server/root_password password letmein | sudo debconf-set-selections' ),
    os.system( 'echo mysql-server mysql-server/root_password_again password letmein | sudo debconf-set-selections' ),
    os.system( 'apt-get -y install mysql-server' ) )

printStep( 'Initializing MySql schema.',
    os.system( 'mysql --user=root --password=letmein < %s' % ( os.path.join( root,
                                                                             'cloud',
                                                                             'schema',
                                                                             'state_db.sql' ), ) ) )

printStep( 'Initializing Cassandra schema.',
    os.system( 'cqlsh < %s' % ( os.path.join( root,
                                              'cloud',
                                              'schema',
                                              'scale_db.cql' ), ) ) )

printStep( 'Installing pip packages for various analytics components.',
    os.system( 'pip install time_uuid cassandra-driver virustotal' ) )

printStep( 'Setting up host file entries for databases locally.',
    os.system( 'echo "127.0.0.1 hcp-state-db" >> /etc/hosts' ),
    os.system( 'echo "127.0.0.1 hcp-scale-db" >> /etc/hosts' ) )

printStep( 'Setting up the C2 keys.',
    os.system( 'ln -s %s %s' % ( os.path.join( root,
                                               'keys',
                                               'c2.priv.pem' ),
                                 os.path.join( root,
                                               'cloud',
                                               'beach',
                                               'hcp' ) ) ) )

printStep( 'Setting up the cloud tags.',
    os.system( 'python %s' % ( os.path.join( root,
                                             'tools',
                                             'update_headers.py' ), ) ) )
