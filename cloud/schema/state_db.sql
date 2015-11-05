CREATE DATABASE hcp;
use hcp;

CREATE TABLE hcp_agentinfo
(
    org TINYINT UNSIGNED NOT NULL,
    subnet TINYINT UNSIGNED NOT NULL,
    uniqueid INT UNSIGNED NOT NULL,
    platform TINYINT UNSIGNED NOT NULL,

    config TINYINT UNSIGNED NOT NULL,

    last_e_ip_A TINYINT UNSIGNED NOT NULL,
    last_e_ip_B TINYINT UNSIGNED NOT NULL,
    last_e_ip_C TINYINT UNSIGNED NOT NULL,
    last_e_ip_D TINYINT UNSIGNED NOT NULL,

    last_i_ip_A TINYINT UNSIGNED,
    last_i_ip_B TINYINT UNSIGNED,
    last_i_ip_C TINYINT UNSIGNED,
    last_i_ip_D TINYINT UNSIGNED,

    last_hostname VARCHAR( 256 ),

    enrollment DATETIME NOT NULL,
    lastseen DATETIME NOT NULL,

    lastmodule VARCHAR( 64 ),
    memused INT UNSIGNED,

    PRIMARY KEY ( org, subnet, uniqueid, platform )

) ENGINE = InnoDB;

CREATE INDEX index_hcp_agentinfo_id ON hcp_agentinfo ( org, subnet, uniqueid, platform );
CREATE INDEX index_hcp_agentinfo_conf ON hcp_agentinfo ( config );




CREATE TABLE hcp_schedule_periods
(
    slice INT UNSIGNED NOT NULL PRIMARY KEY,
    numscheduled INT UNSIGNED

) ENGINE = InnoDB;

CREATE INDEX index_hcp_schedule_periods_num ON hcp_schedule_periods ( numscheduled );




CREATE TABLE hcp_schedule_agents
(
    org TINYINT UNSIGNED NOT NULL,
    subnet TINYINT UNSIGNED NOT NULL,
    uniqueid INT UNSIGNED NOT NULL,
    platform TINYINT UNSIGNED NOT NULL,

    slice INT UNSIGNED NOT NULL,

    PRIMARY KEY ( org, subnet, uniqueid, platform )

) ENGINE = InnoDB;





CREATE TABLE hcp_enrollment_rules
(
    org TINYINT UNSIGNED NOT NULL,
    subnet TINYINT UNSIGNED NOT NULL,
    uniqueid INT UNSIGNED NOT NULL,
    platform TINYINT UNSIGNED NOT NULL,
    config TINYINT UNSIGNED NOT NULL,

    e_ip_A TINYINT UNSIGNED NOT NULL,
    e_ip_B TINYINT UNSIGNED NOT NULL,
    e_ip_C TINYINT UNSIGNED NOT NULL,
    e_ip_D TINYINT UNSIGNED NOT NULL,

    i_ip_A TINYINT UNSIGNED NOT NULL,
    i_ip_B TINYINT UNSIGNED NOT NULL,
    i_ip_C TINYINT UNSIGNED NOT NULL,
    i_ip_D TINYINT UNSIGNED NOT NULL,

    hostname VARCHAR( 256 ) NOT NULL,

    new_org TINYINT UNSIGNED NOT NULL,
    new_subnet TINYINT UNSIGNED NOT NULL,

    PRIMARY KEY ( org, subnet, uniqueid, platform, config, e_ip_A, e_ip_B, e_ip_C, e_ip_D, i_ip_A, i_ip_B, i_ip_C, i_ip_D, hostname, new_org, new_subnet )

) ENGINE = InnoDB;





CREATE TABLE hcp_modules
(
    moduleid TINYINT UNSIGNED NOT NULL,
    hash VARCHAR( 64 ) NOT NULL,
    bin MEDIUMBLOB NOT NULL,
    signature BLOB NOT NULL,
    description VARCHAR( 128 ),

    PRIMARY KEY ( moduleid, hash )

) ENGINE = InnoDB;







CREATE TABLE hcp_module_tasking
(
    org TINYINT UNSIGNED NOT NULL,
    subnet TINYINT UNSIGNED NOT NULL,
    uniqueid INT UNSIGNED NOT NULL,
    platform TINYINT UNSIGNED NOT NULL,
    config TINYINT UNSIGNED NOT NULL,

    moduleid TINYINT UNSIGNED NOT NULL,
    hash VARCHAR( 64 ) NOT NULL,

    PRIMARY KEY ( org, subnet, uniqueid, platform, config, moduleid, hash )

) ENGINE = InnoDB;







CREATE TABLE hcp_agent_reloc
(
    org TINYINT UNSIGNED NOT NULL,
    subnet TINYINT UNSIGNED NOT NULL,
    uniqueid INT UNSIGNED NOT NULL,
    platform TINYINT UNSIGNED NOT NULL,

    new_org TINYINT UNSIGNED NOT NULL,
    new_subnet TINYINT UNSIGNED NOT NULL,

    PRIMARY KEY ( org, subnet, uniqueid, platform )

) ENGINE = InnoDB;



CREATE TABLE hbs_schedule_periods
(
    slice INT UNSIGNED NOT NULL PRIMARY KEY,
    numscheduled INT UNSIGNED

) ENGINE = InnoDB;

CREATE INDEX index_hbs_schedule_periods_num ON hbs_schedule_periods ( numscheduled );



CREATE TABLE hbs_schedule_agents
(
    org TINYINT UNSIGNED NOT NULL,
    subnet TINYINT UNSIGNED NOT NULL,
    uniqueid INT UNSIGNED NOT NULL,
    platform TINYINT UNSIGNED NOT NULL,

    slice INT UNSIGNED NOT NULL,

    PRIMARY KEY ( org, subnet, uniqueid, platform )

) ENGINE = InnoDB;


CREATE TABLE hbs_configurations
(
    org TINYINT UNSIGNED NOT NULL,
    subnet TINYINT UNSIGNED NOT NULL,
    uniqueid INT UNSIGNED NOT NULL,
    platform TINYINT UNSIGNED NOT NULL,
    config TINYINT UNSIGNED NOT NULL,

    modconfigs BLOB NOT NULL,
    originalconfigs BLOB,
    confighash VARCHAR( 64 ),

    PRIMARY KEY ( org, subnet, uniqueid, platform, config )

) ENGINE = InnoDB;


CREATE TABLE hbs_agent_tasks
(
    taskid BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    org TINYINT UNSIGNED NOT NULL,
    subnet TINYINT UNSIGNED NOT NULL,
    uniqueid INT UNSIGNED NOT NULL,
    platform TINYINT UNSIGNED NOT NULL,

    task BLOB NOT NULL,

    PRIMARY KEY ( taskid )

) ENGINE = InnoDB;


