#! /bin/sh

apt-get -y update
apt-get -y install build-essential
apt-get -y install libzmq
apt-get -y install python-pip
apt-get -y install python-dev
apt-get -y install python-gevent
pip install pyzmq
pip install netifaces
pip install pyyaml
pip install psutil
apt-get -y install python-m2crypto
apt-get -y install python-mysqldb
apt-get -y install python-msgpack
pip install pexpect
pip install time_uuid
pip install cassandra-driver
pip install virustotal