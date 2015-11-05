CREATE KEYSPACE "hcp_analytics" WITH REPLICATION = { 'class' : 'SimpleStrategy', 'replication_factor' : 3 };

use hcp_analytics;

CREATE TABLE events(
  eventid varchar,
  event varchar,
  agentid varchar,
  PRIMARY KEY( eventid )
) WITH compaction = { 'class' : 'DateTieredCompactionStrategy', 'timestamp_resolution' : 'SECONDS' } AND gc_grace_seconds = 86400;

CREATE TABLE timeline(
  agentid varchar,
  ts timeuuid,
  eventtype varchar,
  eventid varchar,
  PRIMARY KEY( agentid, ts )
) WITH compaction = { 'class' : 'DateTieredCompactionStrategy', 'timestamp_resolution' : 'SECONDS' } AND gc_grace_seconds = 86400;

CREATE TABLE obj_man
(
  id varchar,

  obj varchar,
  otype int,

  PRIMARY KEY( id )
) WITH CACHING = 'ALL' AND compaction = { 'class' : 'SizeTieredCompactionStrategy' } AND gc_grace_seconds = 86400;

CREATE TABLE obj_name
(
  obj varchar,
  id varchar,
  PRIMARY KEY( obj, id )
) WITH compaction = { 'class' : 'SizeTieredCompactionStrategy' } AND gc_grace_seconds = 86400;

CREATE TABLE loc
(
  id varchar,
  aid varchar,

  otype int,
  last timestamp,

  PRIMARY KEY( aid, otype, id )
) WITH compaction = { 'class' : 'LeveledCompactionStrategy' } AND gc_grace_seconds = 86400;

CREATE TABLE loc_by_id
(
  id varchar,
  aid varchar,
  last timestamp,
  PRIMARY KEY( id, aid )
) WITH compaction = { 'class' : 'LeveledCompactionStrategy' } AND gc_grace_seconds = 86400;

CREATE TABLE loc_by_type
(
  d256 int,
  otype int,
  id varchar,
  aid varchar,
  PRIMARY KEY( d256, otype, id, aid )
) WITH compaction = { 'class' : 'LeveledCompactionStrategy' } AND gc_grace_seconds = 86400;

CREATE TABLE rel_man_parent
(
  parentkey varchar,
  ctype int,
  cid varchar,
  PRIMARY KEY( parentkey, ctype, cid )
) WITH compaction = { 'class' : 'LeveledCompactionStrategy' } AND gc_grace_seconds = 86400;


CREATE TABLE rel_man_child
(
  childkey varchar,
  ptype int,
  pid varchar,
  PRIMARY KEY( childkey, ptype, pid )
) WITH compaction = { 'class' : 'LeveledCompactionStrategy' } AND gc_grace_seconds = 86400;


CREATE TABLE recentlyActive
(
  agentid varchar,
  last timestamp,
  PRIMARY KEY( agentid )
) WITH compaction = { 'class' : 'LeveledCompactionStrategy' } AND gc_grace_seconds = 86400;

CREATE TABLE last_events
(
  agentid varchar,
  type varchar,
  id varchar,
  PRIMARY KEY( agentid, type )
) WITH compaction = { 'class' : 'LeveledCompactionStrategy' } AND gc_grace_seconds = 86400;

CREATE TABLE investigation_data
(
  invid varchar,
  ts timeuuid,
  eid uuid,
  etype varchar,
  PRIMARY KEY( invid, ts )
) WITH compaction = { 'class' : 'SizeTieredCompactionStrategy' } AND gc_grace_seconds = 86400;

CREATE TABLE ref_loc
(
  id varchar,
  plat int,
  nloc int,

  PRIMARY KEY( id, plat )
) WITH compaction = { 'class' : 'LeveledCompactionStrategy' } AND gc_grace_seconds = 86400;