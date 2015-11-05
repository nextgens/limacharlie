# LIMA CHARLIE
![LIMA CHARLIE](https://raw.github.com/refractionPOINT/limacharlie/master/doc/lc.png)

*Stay up to date with new features and detection modules: [@rp_limacharlie](https://twitter.com/rp_limacharlie)*
*Need help? Contact other LC users via the Google Group: https://groups.google.com/d/forum/limacharlie*
*For more direct enquiries, contact us at limacharlie@refractionpoint.com*

## Overview
LIMA CHARLIE is an endpoint security platform. It is itself a collection of small projects all working together
to become the LC platform. LC gives you a cross-platform (Windows, OSX, Linux, Android and iOS) low-level 
environment allowing you to manage and push (in memory) additional modules to. The main module (at the moment) 
is the HBS sensor, which provides telemetry gathering and basic forensic capabilities.
 
Many of those individual features are provided through other platforms, so why LC? LC gives you a single 
messaging, cloud and analytic fabric that will integrate with anything and scale up. Sensor is extra-light
and installs nothing on the host.

Ultimately LC is meant to be a platform for the security community to experiment with, a starter kit to have the 
endpoint monitoring you want or to the platform enabling you to try new endpoint techniques without the hassle of
rebuilding the basics.

### HCP vs HBS
Simply put, HCP is the base sensor that can load additional modules in memory, takes care of crypto, message 
and beaconning, but nothing security-related. HBS is the security part of LC, it's the sensor that gets loaded as a module by HCP.

### Capabilities

#### In-Sensor

Not all capabilities listed here are available for every OS. Know how to integrate in a new OS, please contribute!

##### Exfil
We wouldn't mention the exfil of events as a capability, but it's awesome in LC. The sensor can be configured
at runtime (through a Profile) to exfil different events. The events are always generated (and used) within the
sensor, but sending it all to the cloud would be a lot of data. The cloud has the capability to select which
ones to send back. This means you can exfil the basic most of the time, but if the cloud detects something 
suspicious, it can tell the sensor to start sending a bunch more events back, from file io to individual module
loads.

##### Process Tracking
Simple, a process starts or ends, an event with the basic information about it is generated.

##### DNS Tracking
Simple, a DNS request is made, an event with the basic information is generated.

##### Code Identity
Simple, when a piece of code is loaded, basic information like hash is gathered. An event is only generated
on the first time that sensor (at that boot) sees that specific piece of code.

##### Network Tracking
Simple, essentially netflow, an event is generated for every connection change.

##### Hidden Module Detection
Looks for hidden modules loaded in memory in a few different ways.

##### Module Tracking
When a module (dll) is loaded, an event is generated.

##### File Tracking
When a file is created, renamed or deleted from the main drive, an event is generated.

##### Network / Process Summarization
Netflow connections are associated with processes, when the process ends or when a maximum number of connections
is reached, we generate an event summarizing everything we know.

##### File Forensics
All the basic file operations are available programmatically, more fancy operations like MFT reading to come. 

##### Memory Forensics
All the basic memory forensic operations are available. From dumping specific areas of memory to listing strings
and handles in memory.

##### OS Forensics
Queries to list various OS-specific characteristics like Services, Drivers and Autoruns.

##### Detailed History on Demand
All events are generated internally on the sensor, some are sent back, but we keep a rolling buffer of the latest
full spread of events. This rolling buffer can be flushed and sent to the cloud on demand.

#### In-Cloud

##### Stateless

These are extremely lightweight python classes. This is where the community can contribute the most in the easiest
ways. Got an idea? Try it, share it!

##### BatchSelDelete
Simple module looking for a command line pattern that looks like the self-delete technique on Windows that
drops a batch file polling to delete the main dropper.

##### FirewallCliMods
Looks for command line patterns of someone adding a new firewall rule on Windows through the CLI util.

##### KnownObjects
This module refreshes a list of of known-bad Objects (LC Objects, see *Object Modelization* section) periodically and then looks for them through
all the events coming back. This is a modular detection as it relies on a different Actor to provide it with
a list of known-bad, so roll your own from a threat feed you have or some other open sources.

##### SuspExecLoc
Looks for execution in a list of directories where things should never execute from (like the Recycler on Windows).

##### VirusTotal
Queries all hashes seen in incoming events to VirusTotal and reports when a hash has more than X AVs saying it's bad.

##### Stateful
... Upcoming ... Widget-based stateful detection across all events types in time and content.

##### Object Modelization
We extract Objects from all events coming back from the sensors. Objects are characteristics of telemtry for example:
- RELATION
- FILE_PATH
- FILE_NAME
- PROCESS_NAME
- MODULE_NAME
- MODULE_SIZE
- FILE_HASH
- HANDLE_NAME
- SERVICE_NAME
- CMD_LINE
- MEM_HEADER_HASH
- PORT
- THREADS
- AUTORUNS
- DOMAIN_NAME
- PACKAGE
- STRING
- IP_ADDRESS

We do a few different things with these Objects.

The events are parsed and passed along to all the detection modules. This means a module you write doesn't have to
know about the details of all the event types it's looking at to be able to evaluate parts of their content.

We also ingest all the Objects into the scale-db. But we don't stop there, when we ingest them, we use a very enhanced
model where we store the Object itself and also the Relationships of that Object (which are Objects themselves) and
we pivot all this in a bunch of ways that allow you to browse, get statistics and build context around them.

For a basic example, in a process listing you may get the following:
- Object( 'iexplore.exe', PROCESS_NAME )
- Object( 'wininet.dll', MODULE_NAME )
- Relation( 'iexplore.exe', PROCESS_NAME, 'wininet.dll', MODULE_NAME )

And in a network-summary:
- Object( 'iexplore.exe', PROCESS_NAME )
- Object( '80', PORT )
- Relation( 'iexplore.exe', PROCESS_NAME, '80', PORT )

## Getting Started
This is easy, as a test bed, you can run the whole thing in a VM with 1 GB of RAM and a single CPU, of course
you'll want more beefy nodes for a large deployment. All development for LC is done primarily on Debian (Ubuntu)
so if you want to run on RH, some small tweaks might be needed, in which case please let us know and we'll merge
into our main repo to make it easier on other users.

A quick note on infrastructure segments: LC is very modular, which means if you'd like to run the LC sensors but
want to roll in all the data into your SIEM, or Splunk or something, it's very easy to leave the analytic components
of the LC infra behind, you will simply ommit to run a few Actors and will run your own instead.

### Cloud-in-a-Can
The steps below show how to create your own cloud, but if all you want is to give it a quick try for now, there
is another way. This quick-setup will completely automate the installation and configuration of a cloud using
default keys, it won't be good to scale up in a production environment, but will get you running and testing sensors
in minutes.

1. Setup a host with at least 1 GB of RAM and install Ubuntu Server on it (other Debian may be good but your
mileage may vary.
1. Copy the git repository somewhere on the host.
1. In the repo, go to: `cloud/infrastructure/`.
1. Execute `sudo python 1_install_single_node_test_cluster.py` to install all required packages.
1. Execute `sudo python 2_start_single_node_test_cluster.py` to start the Beach cluster and LC Actors.
1. Execute `sudo python 3_configure_single_node_test_cluster.py` to configure the LC cloud, task sensors etc.
1. Run the standalone HCP sensors found in `binary_releases/`, making sure that a DNS or host file entry on the
hosts running the sensors "rp_c2_dev" points that new cloud box.

### Generating Keys
First step, although not as exciting is generating new keys. Now, test keys are included in the repo so you don't
**have** to do it, but unless it's a truly temporary test running on a test VM I'd **strongly** recommend using 
your own keys. If you use the test keys, remember that anyone can decrypt, task and intercept your sensors.

To generate a new key set (we use RSA 2048) from the root of the repo:

```
cd tools
python ./generate_key.py <keyName>
```

That will create a private and public key in a few different formats (but all the same key pair).

You need to generate 3 such key pairs:
- HCP: this pair is used to sign the modules we will load on the platform, in our case this will be HBS.
- C2: this is a least-privileged pair used to secure the link between the Command & Control for LC and the sensors.
- HBS: this is a pair used to sign the tasking we send to the HBS sensor.

The security of the HCP and HBS keys is critical as they could allow an attacker to "do stuff" to your sensors. The
C2 key on the other hand is not as critical since it allows only very basic "keeping the lights on" functionality
to the sensor. This is done on purpose. The entire LC model is done such that you could keep the critical keys
(HCP and HBS) in an air-gap and still be able to run LC.

Last step is to encrypt the HBS key. This will allow you to use the key in the LC Shell, which needs a key with
a password for added safety (since you use the HBS key more often and are less likely to completely air-gap it).

```
cd tools
./encrypt_key.sh <hbsPrivateKeyDer>
```

Now you're done, copy all the keys somewhere safe. Once we have our cloud running you will need the *C2 key public pem*
on your cloud and will reference to it from an Actor you start, we'll explain later.

### Installing Backend

#### Beach Cluster
The cloud runs on a Beach cluster. Beach is Python based which makes development of all the analytics easier and
the Beach cluster can be expanded at runtime (it's peer-to-peer) which makes growing your deployment easier in the
future. So as not to duplicate doc, check out the *Basic Usage* section here: https://github.com/refractionPOINT/py-beach/

A one-node cluster is perfectly fine to do the testing of LC.

Copy the LC repo (whole repo is easier, but really it's the *cloud/beach* directory you need) to your Beach cluster. This
can be done manually, through a NFS-mounted directory or a config management system like Salt, Chef etc...

Point your Beach cluster config's data directory to the *cloud/beach* directory of the repo. Alternatively, use 
the sample Beach config found in the *cloud/beach* directory in the repo, it should have all the relative directories
pointed correctly. The *realm* we will use in the Beach cluster is called *hcp*, but you don't really need to know
that for now.

On each node of the Beach cluster, install the required packages. The required packages can be found in the repo:
```
cloud/beach/requirements.sh
```

or in a sample Salt config:
```
cloud/infrastructure/salt/limacharlie_deps.sls
```

#### Setup Http Endpoint


#### State DB
The state-db is a MySql database we use as a simple scratch-space for ongoing "keeping the lights on" in the 
Command & Control. So install a MySql database server (wherever you want) that is reachable from your Beach cluster.
Setup the user accounts you'd like (or not) and finally create the schema found here:
```
cloud/schema/state_db.sql
```

#### Scale DB
The scale-db is a Cassandra cluster (again, one-node cluster for testing is fine). Cassandra will provide you with
highly-scalable storage, which is something you will need in order to run some of the analytics (especially Objects)
on a any real deployment. As with the state-db, just install Cassandra somewhere reachable to the Beach cluster and
create the schema found here:
```
cloud/schema/scale_db.cql
```

### Configuring Backend

#### Set the Keys
Using the keys you generated earlier, copy the C2 public pem key to the cloud/beach/hcp/ directory on the Beach 
cluster and make sure the key name matches the name given in the parameters of the BeaconProcess Actor. In the
sample_start.py the default name is c2.priv.pem.

#### Start the Actors
Now that we have all the raw components required ready, let's load the code onto the cluster.

To load the Actors that form the backend needed for LC, run the sample_start.py script from a location having
access to the beach cluster (or from a node itself).

```
cd cloud/beach
python ./sample_start.py
```

The sample_start.py script contains documentation about which Actor does what, which will allow you to customize
the composition of your production cloud infra for LC (if you need). In its current form, the sample_start.py will
start one Actor of each required Actor for LC to work, including analytics.

#### Add Enrollment Rule
Now we want to add an enrollment rule. Because LC was designed to be able to cope with the challenges
of large organizations, we need to set a rule that describes what sensor id range a new sensor reporting in should
be associated to. Sensor ids are a 5-tuple and have the following structure:
- Org Id (1 byte): allows a cloud to house multiple organizations
- Subnet Id (1 byte): further segmentation within an org
- Unique Id (4 bytes): this is a one-up number for each new sensor within the subnet
- Platform Id (1 byte split in 3 fields):
  - CPU Arch (2 bits)
    - 0 : reserved
    - 1 : 32 bit x86
    - 2 : 64 bit x64
    - 3 : mask value
  - Major version (3 bits):
    - 0 : reserved
    - 1 : Windows
    - 2 : OS X
    - 3 : IOS
    - 4 : Android
    - 5 : Linux
    - 6 : reserved
    - 7 : mask value
  - Minor version (3 bits): depends on the major, 7 is mask value
- Config Id (1 byte): runtime specifiable value, often ignored other than for specific deployment scenarios 

Sensor ids support masking, which means that the following expression represents all Windows 64 bit regarless
of organization and subnet: ff.ff.ffffffff.21f.ff

So for this simple testing, we'll create a rule that says "enroll all sensors into the same range, org=1 subnet=1".

First, to interact with the cloud, we will start the LC Shell. Start by configuring a shell config file, for this
you can use the one in *cloud/beach/sample_cli.conf*. If you need, edit it and make the *beach_config* parameter
point to your Beach cluster config file, it's the only parameter required for the moment. Eventually we will
configure different access tokens in that config file, so you can think of it as a per-operator config file.

Now that this is ready, start the shell:
```
python cloud/beach/hcp/admin_cli.py
```

Log in to the cloud with the cli config file from a paragraph ago.
```
login cloud/beach/sample_cli.conf
```

It should tell you you've successfully logged in. This shell supports the '?' command and you can give commands the
'-h' flag to get a more detailed help. If you check out the '-h' for the 'login' command, you'll see it supports
the '-k keyFile' parameter. If you login without '-k', you can issue commands to the cloud and manage it, but you
cannot issue commands to the actual sensors. By loading the key (HBS private key you encrypted earlier), you can 
start issuing commands directly to sensors. When a key is loaded, a star will be displayed to the left in the CLI.

Back to the enrollment, add the following rule:
```
hcp_addEnrollmentRule -m ff.ff.ffffffff.fff.ff -o 1 -s 1
hcp_getEnrollmentRules
```

Done, now new sensors coming in will get officially enrolled.

#### Add Profile

A Profile is a configuration for sensors, where we can configure the events exfiled and many other things. For
sensors to bring back anything, we must give them a profile, so let's add one, like the enrollment rule, for
every sensor we deal with. Notice that the Profile applies to the security sensor (HBS) and therefore the command
starts with *hbs_*.

```
hbs_addProfile -m ff.ff.ffffffff.fff.ff -f cloud/beach/sample_hbs.profile
```

You can generate new profiles by using the *cloud/beach/generate_sensor_profile.py*, but for now the sample should
do.

### Generating Installers
Great, the cloud is done, now we need to prepare installers for sensors. You can compile your own sensors if you'd
like, but we also release binary versions of vanilla sensor at major releases. The general procedure is to compile
a sensor (or download an official release), and then you patch it with a config so it uses your keys and settings.
If you don't patch it, the sensor will remain a "dev" sensor and will use demo keys and connect to a local cloud.
 
To generate a new sensor config, you can use *cloud/beach/generate_sensor_config.py*, but for now, we'll use the
*cloud/beach/sample_hcp_sensor.conf*. Before we patch, edit the *sample_hcp_sensor.conf* and change the *C2_PUBLIC_KEY* and 
*ROOT_PUBLIC_KEY* to the path to your C2 public key and HCP public key.
 
So to apply (patch) a sensor config, we use *cloud/beach/set_sensor_config.py*:
```
python set_sensor_config.py <configFile> <sensorExecutable>
```

The executable is now patched, it's got your keys and is ready to execute.

### Running Installers
NSIS installers are included in the repository but have yet to be revamped and therefore your mileage may vary.
For the moment your best bet is running the sensors manually as standalone executables with root privileges.

Since no component is installed permanently by LC (other than a tiny config file), distributing and running
the standalone versions of the HCP sensor is not problematic. HCP should not change (or very rarely) and HBS
can be updated from the cloud transparently. On Windows deploying HCP via Microsoft GPO is generally a good way
of mass deploying and undeploying HCP easily.

### Confirm Sensors are Running
The sensor when launched as a service or executable does not fork so a simple "ps" should tell you if it's running.
On the Beach cluster, you should see activity in the syslog.
Finally, by pointing your browser to the LC dashboard, you should now see your sensors connecting in.

### Patch, Sign, Load and Task HBS
So now we have HCP sensors coming back to our cloud, but we're not pushing any actual sensors to them (HBS).
To remedy that, we'll prepare an HBS sensor and then task it to our sensors.

The patching process is the same as for HCP, except that in this case you want to patch it with a different set
of configs, mainly with a root HBS key which is used to send cloud events back to the sensor. Those events being
sent back down need to be signed with this key. So go ahead and use `set_sensor_config.py sample_hbs_sensor.conf 
<hbsBinary>`.

Now that HBS has the config we want, we need to sign it with the root HCP key we generated. To do this, use
```
python tools/signing.py -k rootHcpKey.der -f path/to/hbs/binary -o path/to/hbs/binary.sig
```

This generates a .sig file wich contains the signature of the HBS module by the root HCP key, and thus allows it
to be loaded by HCP.

Next we'll task the HBS sensor to HCP. This is done in two steps through the `admin_cli.py`, first we upload the
HBS module:
```
hcp_addModule -i 1 -b path/to/hbs/binary -s path/to/hbs/binary.sig -d "this is a test hbs"
```

note the hash string given as identifier for the module you just uploaded, it's just the sha-1 of the HBS module file.
Then we'll actually task the module we uploaded to the right sensors, in this case to every sensor:
```
hcp_addTasking -i 1 -m ff.ff.ffffffff.fff.ff -s hashOfTheModuleUploaded
```

From this point on, when the sensor contacts the cloud, the cloud will instruct it to load this HBS module.

### Enjoying
Excellent, by now you should be set and be able to see the basic information in the LC web UI.

## Extending
More doc and guides to follow...

### Sensor
#### HCP Module
The sensor is written in C. HCP (base platform) supports multiple modules, if you'd like to write your own
I suggest having a look at the rpHCP_TestModule in /sensor/executables/. Modules can be loaded in parallel
so you having your own module does not mean it replaces HSB necessariliy.

#### HBS Collector
Expanding on HBS is much easier. Have a look at /sensor/executables/rpHCP_HostBasedSensor/main.c at the top
where Collectors are defined. Addind your own collector is very well scoped and should provide you with
all the functionality you need for a security sensor.

### Cloud
#### Stateless Detection
If the integration you have in mind can operate on a per-event basis, a stateless actor is the easiest way
to do it. Start by copying one in place in /cloud/beach/hcp/analytics/stateless/ and putting it in the same
directory. A stateless actor requires an `init`, `deinit` (optional) and `process` function. Since stateless
actors are so simple, it's better to simply have a look at a few and replicate in your own.

#### Stateful Detection
Stateful detection is more complex and remains to be completely brought in, more doc to come.

### Reporting & Auditing
Reporting of detects is currently left to the user to implement, everyone has a different SIEM or detection
pipeline. The detect reports are sent to the `analytics/report` Beach category, a stub actor to receive them
is implemented as AnalyticsReporting.py.

### Modeling
The modeling uses Cassandra as a main data store, which implies some particularities in the way the schema is
designed. If you've never used NoSql databases like Cassandra, it's recommended that you read [this](http://www.datastax.com/dev/blog/basic-rules-of-cassandra-data-modeling).

Beyond the datastore and using the AnalyticsModeling Actor as an example, there is no further framework to help.

## Components
Below is a breakdown of the main components of LIMA CHARLIE.

### RPAL
Stands for Refraction Point Abstraction Library. It is a static library providing low level abstraction across
all supported operating systems. All other components are based on it. It provides an abstraction of
- Basic data types, gives you a common definition for things like 32 bit int etc.
- Standard (or not so much) libraries, smooths out common functions like strlen that should be standard but are not always.
- Common data structures, providing staples of programming like b-trees, blobs etc.

### RPCM
Stands for Refraction Point Common Messaging. It is a static library that provides a serializable and cross-platform
message structure in a similar way as protobuf, but with a nicer C interface and without enforced schema. Has a C and
a Python API.

### HCP
Stands for Host Common Platform, it is the platform residing on disk on the endpoints. It is responsible for crypto,
comms and loading modules sent by the cloud. It doesn't really "do" anything else, it's the base platform that gives
us flexibility in what we actually want to deploy.

### HBS
Stands for the Host Based Sensor. It is the "security monitoring" part of Lima Charlie. It's a module loadable by
HCP, so it is tasked from the cloud and does not persist on the endpoints, making it easier to manage upgrades 
across the fleet.

## Architecture
![Architecture](https://raw.github.com/refractionPOINT/limacharlie/master/doc/architecture.png)

## Crypto Talk
### Algorithms
AES-256 is used for the comms channel to the command and control.

RSA-2048 is used to exchage the session key for the comms channel. It is also used to sign the taskings and modules.

SHA-256 is used as general hashing algorithm and for signatures. 

### Keys
#### C2
This key pair is used to secure the session key exchange with the command and control. It is NOT privileged in HCP
which means its compromise is important but not critical. An attacker will not be able to task the sensors with it.
It means it can safely sit in the cloud with BeaconProcessor Actors.

#### HCP
The HCP key pair is used to sign the modules loaded on HCP. This means a compromise of this key is critical since
it could allow an attacker to load arbitrary code in HCP. For this reason it's recommended to keep it offline and
sign new module (like HBS) releases on an air-gapped trusted system, and copy the resulting `.sig` signatures back
to your cloud once signed.

#### HBS
The HBS key pair is used to issue taskings to the HBS module. Compromise is also critical as it can allow an attacker
to modify the host. This key pair is NOT required to be "live" in the cloud. For this reason it is recommended that
key pair is kept in as much of an air-gap as possible and only loaded in the `admin_cli.py` only when required. The
`admin_cli.py` loads this key assuming it is protected (see Generating Keys above) and decrypts and loads them in 
memory, meaning you can (for example) keep the key on a USB stick and connect it only at the start time of the cli and
then disconnect, limiting the exposure of the keys.

## Building

### Windows
To be able to maintain easy backwards compatibility with mscrt, building on Windows will require Visual Studio
and version 3790.1830 of the DDK.
- Install the DDK to %systemdrive%\winddk\3790.1830\
- Clone the Lima Charlie repository locally on your Windows box
- Run Visual Studio as Administrator (not 100% required but makes debugging easier since LC is designed to run as
admin)
- In Visual Studio, open the solution in `sensor/solutions/rpHCP.sln`
- When/if prompted, do NOT upgrade `.vcxproj` project files to a newer version as it may break some of the trickery
 used to build with the DDK
- You should be ready to build

### Nix
It's simpler, cd to the sensor/ directory and run `scons`.

## Utilities
### HBS Profile
Configuring which collectors are running, which events are sent back to the cloud or configure specific parameters
for a collector, use the `/cloud/beach/generate_sensor_profily.py`. If you'd like to understand and tweak the exact
parameters, you may have a look at the `/cloud/beach/sample_hbs.profile` which uses the RPCM Python API directly,
it's what the generator python script produces anyway.

### Update Headers
If you're expanding on the endpoint side, you will likely want to add new tags. To add a new tag, add it to the base
JSON file `/meta_headers/rp_hcp_tags.json` and then call `/tools/update_headers.py`. This last script will take the
tag definition you've added and will generate the various C and JSON headers for the other components of the system.

## LIMA CHARLIE WEB UI
The LC web UI is not meant to be the end-all-be-all for LC, but rather a simple UI to get you started visualizing
the data coming back from the endpoints and even exploring some of the pivoted Object data. It is in itself quite
useful, but it's not a SIEM. The logical place for a SIEM is not the event stream itself, it's the Detects generated
since this is where the "signals" are generated.

All this being said, future more advanced capabilities are likely to use the UI as a proof of concept.

## Next Steps
Please, feel at home contributing and testing out with the platform, it's what its for. HBS currently has some limitations
, mainly around being User Mode only. User Mode is great for stability (not going to blue screen boxes) and speed
of prototyping, but it lacks many of the low level event-driven APIs to do things like getting callbacks for processes
creation. So here are some of the capabilities that are coming:
- Kernel Mode thin module providing low level events and APIs.
- Code injection detection through stack introspection.
- Stateful detections.