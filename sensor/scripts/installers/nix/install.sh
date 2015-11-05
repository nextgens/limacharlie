#! /bin/bash

if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# Some environments are bare-metal in sudo
export PATH=$PATH:/sbin

SVC_NAME='rphcp'
CORE_SIZE="0"
if [ "`cat build`" = "debug" ]
then
  CORE_SIZE="unlimited"
fi

if [ "$1" = "uninstall" ];
then

  echo "Uninstalling ${SVC_NAME}."

  if [ "`which chkconfig`" ]
  then
    echo "::: CENTOS"
    service ${SVC_NAME}d stop
    chkconfig --level 345 ${SVC_NAME}d off
    chkconfig --del ${SVC_NAME}d
    rm /etc/init.d/${SVC_NAME}d
  elif [ "`which update-rc.d`" ]
  then
    echo "::: Ubuntu"
    rm /etc/init.d/${SVC_NAME}d
    update-rc.d ${SVC_NAME}d remove
  elif [ "`which launchd`" ]
  then
    echo "::: MacOsX"
    launchctl unload com.refractionpoint.rphcp
    rm /Library/LaunchDaemons/com.refractionpoint.rphcp.plist
  else
    echo "Unknown, unregister manually"
  fi

  rm /bin/$SVC_NAME

else

  echo "Installing $SVC_NAME."

  if [ -z "$1" ]; then
    echo "No license key given on command line. Abort."
    exit 1
  fi
  EXPECTED_KEY=`cat ./license-key`
  if [ "$EXPECTED_KEY" != "$1" ]; then
    echo "Given license key '$1' is different from the one you were supplied. Abort."
    exit 1
  fi

  echo "...svc bin"
  cp ./bin /bin/$SVC_NAME

  echo "...svc script"
  if [ "`which launchd`" ]
  then
    cp ./launchd_svc /Library/LaunchDaemons/com.refractionpoint.rphcp.plist
    chmod 700 /Library/LaunchDaemons/com.refractionpoint.rphcp.plist
  else
    awk "/%CORE_SIZE%/{ sub( /%CORE_SIZE%/, \"$CORE_SIZE\" ); } 1" ./svc > /etc/init.d/${SVC_NAME}d
    chmod 754 /etc/init.d/${SVC_NAME}d
  fi

  echo "...registering"
  if [ "`which chkconfig`" ]
  then
    echo "::: CentOS"
    chkconfig --add ${SVC_NAME}d
    chkconfig --level 345 ${SVC_NAME}d on
    service ${SVC_NAME}d start
  elif [ "`which update-rc.d`" ]
  then
    echo "::: Ubuntu"
    update-rc.d ${SVC_NAME}d defaults
    service ${SVC_NAME}d start
  elif [ "`which launchd`" ]
  then
    echo "::: MacOSX"
    launchctl load /Library/LaunchDaemons/com.refractionpoint.rphcp.plist
    launchctl start com.refractionpoint.rphcp
  else
    echo "Unknown, register manually"
  fi

fi

echo "done"

# EOF

