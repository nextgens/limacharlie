import os
import sys
from beach.beach_api import Beach
import logging

if 1 < len( sys.argv ):
    BEACH_CONFIG_FILE = os.path.abspath( sys.argv[ 1 ] )
else:
    BEACH_CONFIG_FILE = os.path.join( os.path.dirname( os.path.abspath( __file__ ) ), 'sample_cluster.yaml' )

beach = Beach( BEACH_CONFIG_FILE, realm = 'hcp' )

#######################################
# BeaconProcessor
# This actor will process incoming
# beacons from the sensors.
# Parameters:
# state_db: these are the connection
#    details for the mysql database
#    used to store the low-importance
#    data tracked at runtime.
#######################################
print( beach.addActor( 'c2/BeaconProcessor',
                       'c2/beacon/1.0',
                       parameters = { 'state_db' : { 'url' : 'hcp-state-db',
                                                     'db' : 'hcp',
                                                     'user' : 'root',
                                                     'password' : 'letmein' },
                                      'priv_key' : os.path.join( os.path.dirname( os.path.abspath( __file__ ) ),
                                                                 'hcp',
                                                                 'c2.priv.pem' ) },
                       secretIdent = 'beacon/09ba97ab-5557-4030-9db0-1dbe7f2b9cfd',
                       trustedIdents = [ 'http/5bc10821-2d3f-413a-81ee-30759b9f863b' ],
                       n_concurrent = 5 ) )

#######################################
# AdminEndpoint
# This actor will serve as a comms
# endpoint by the admin_lib/cli
# to administer the LC.
# Parameters:
# state_db: these are the connection
#    details for the mysql database
#    used to store low-importance
#    data tracked at runtime.
#######################################
print( beach.addActor( 'c2/AdminEndpoint',
                       'c2/admin/1.0',
                       parameters = { 'state_db' : { 'url' : 'hcp-state-db',
                                                     'db' : 'hcp',
                                                     'user' : 'root',
                                                     'password' : 'letmein' } },
                       secretIdent = 'admin/dde768a4-8f27-4839-9e26-354066c8540e',
                       trustedIdents = [ 'cli/955f6e63-9119-4ba6-a969-84b38bfbcc05' ],
                       n_concurrent = 5 ) )

###############################################################################
# Analysis Intake
###############################################################################

#######################################
# AnalyticsIntake
# This actor receives the messages from
# the beacons and does initial parsing
# of components that will be of
# interest to all analytics and then
# forwards it on to other components.
#######################################
print( beach.addActor( 'analytics/AnalyticsIntake',
                       'analytics/intake/1.0',
                       parameters = {  },
                       secretIdent = 'intake/6058e556-a102-4e51-918e-d36d6d1823db',
                       trustedIdents = [ 'beacon/09ba97ab-5557-4030-9db0-1dbe7f2b9cfd' ],
                       n_concurrent = 5 ) )

#######################################
# AnalyticsModeling
# This actor is responsible to model
# and record the information extracted
# from the messages in all the different
# pre-pivoted databases.
# Parameters:
# db: the Cassandra seed nodes to
#    connect to for storage.
# rate_limit_per_sec: number of db ops
#    per second, limiting to avoid
#    db overload since C* is bad at that.
# max_concurrent: number of concurrent
#    db queries.
# block_on_queue_size: stop queuing after
#    n number of items awaiting ingestion.
#######################################
print( beach.addActor( 'analytics/AnalyticsModeling',
                       'analytics/modeling/intake/1.0',
                       parameters = { 'db' : [ 'hcp-scale-db' ],
                                      'rate_limit_per_sec' : 500,
                                      'max_concurrent' : 10,
                                      'block_on_queue_size' : 200000 },
                       secretIdent = 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5',
                       trustedIdents = [ 'intake/6058e556-a102-4e51-918e-d36d6d1823db' ],
                       n_concurrent = 5,
                       isIsolated = True ) )

#######################################
# AnalyticsStateless
# This actor responsible for sending
# messages of the right type to the
# right stateless detection actors.
#######################################
print( beach.addActor( 'analytics/AnalyticsStateless',
                       'analytics/stateless/intake/1.0',
                       parameters = {  },
                       secretIdent = 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5',
                       trustedIdents = [ 'intake/6058e556-a102-4e51-918e-d36d6d1823db' ],
                       n_concurrent = 5 ) )

#######################################
# AnalyticsStateful
# This actor responsible for sending
# messages of the right type to the
# right stateful detection actors.
#######################################
print( beach.addActor( 'analytics/AnalyticsStateful',
                       'analytics/stateful/intake/1.0',
                       parameters = {  },
                       secretIdent = 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5',
                       trustedIdents = [ 'intake/6058e556-a102-4e51-918e-d36d6d1823db' ],
                       n_concurrent = 5 ) )

#######################################
# AnalyticsReporting
# This actor receives Detecs from the
# stateless and stateful detection
# actors and ingest them into the
# reporting pipeline.
#######################################
print( beach.addActor( 'analytics/AnalyticsReporting',
                       'analytics/reporting/1.0',
                       parameters = {  },
                       secretIdent = 'reporting/9ddcc95e-274b-4a49-a003-c952d12049b8',
                       trustedIdents = [ 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5' ],
                       n_concurrent = 5 ) )

#######################################
# ModelView
# This actor is responsible to query
# the model to retrieve different
# advanced queries for UI or for
# other detection mechanisms.
# Parameters:
# db: the Cassandra seed nodes to
#    connect to for storage.
# rate_limit_per_sec: number of db ops
#    per second, limiting to avoid
#    db overload since C* is bad at that.
# max_concurrent: number of concurrent
#    db queries.
# block_on_queue_size: stop queuing after
#    n number of items awaiting ingestion.
#######################################
print( beach.addActor( 'analytics/ModelView',
                       'models/1.0',
                       parameters = { 'scale_db' : [ 'hcp-scale-db' ],
                                      'state_db' : { 'url' : 'hcp-state-db',
                                                     'db' : 'hcp',
                                                     'user' : 'root',
                                                     'password' : 'letmein' },
                                      'rate_limit_per_sec' : 500,
                                      'max_concurrent' : 10,
                                      'beach_config' : BEACH_CONFIG_FILE },
                       trustedIdents = [ 'lc/0bf01f7e-62bd-4cc4-9fec-4c52e82eb903' ],
                       n_concurrent = 5 ) )

###############################################################################
# Stateless Detection
###############################################################################

#######################################
# stateless/SuspExecLoc
# This actor looks for execution from
# various known suspicious locations.
#######################################
print( beach.addActor( 'analytics/stateless/SuspExecLoc',
                       'analytics/stateless/suspexecloc/1.0',
                       parameters = {  },
                       secretIdent = 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5',
                       trustedIdents = [ 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5' ],
                       n_concurrent = 5 ) )

#######################################
# stateless/BatchSelfDelete
# This actor looks for patterns of an
# executable deleteing itself using
# a batch script.
#######################################
print( beach.addActor( 'analytics/stateless/BatchSelfDelete',
                       'analytics/stateless/batchselfdelete/1.0',
                       parameters = {  },
                       secretIdent = 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5',
                       trustedIdents = [ 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5' ],
                       n_concurrent = 5 ) )

#######################################
# stateless/KnownObjects
# This actor looks for known Objects
# in all messages. It receives those
# known Objects from another actor
# source that can be customized.
# Parameters:
# source: the beach category to query
#    to receive the known Objects.
# source_refresh_sec: how often to
#    refresh its known Objects from
#    the source.
#######################################
print( beach.addActor( 'analytics/stateless/KnownObjects',
                       'analytics/stateless/knownobjects/1.0',
                       parameters = { 'source' : 'sources/known_objects/',
                                      'source_refresh_sec' : 3600 },
                       secretIdent = 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5',
                       trustedIdents = [ 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5' ],
                       n_concurrent = 5 ) )

#######################################
# stateless/VirusTotal
# This actor checks all hashes against
# VirusTotal and reports hashes that
# have more than a threshold of AV
# reports, while caching results.
# Parameters:
# key: the VT API Key.
# qpm: maximum number of queries to
#    to VT per minute, based on your
#    subscription level, default of 4
#    which matches their free tier.
# min_av: minimum number of AV reporting
#    a result on the hash before it is
#    reported as a detection.
# cache_size: how many results to cache.
#######################################
print( beach.addActor( 'analytics/stateless/VirusTotal',
                       'analytics/stateless/virustotal/1.0',
                       parameters = {  },
                       secretIdent = 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5',
                       trustedIdents = [ 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5' ],
                       n_concurrent = 1 ) )

#######################################
# stateless/FirewallCliMods
# This actor looks for patterns of an
# executable adding firewall rules
# via a command line interface.
#######################################
print( beach.addActor( 'analytics/stateless/FirewallCliMods',
                       'analytics/stateless/firewallclimods/1.0',
                       parameters = {  },
                       secretIdent = 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5',
                       trustedIdents = [ 'analysis/038528f5-5135-4ca8-b79f-d6b8ffc53bf5' ],
                       n_concurrent = 5 ) )