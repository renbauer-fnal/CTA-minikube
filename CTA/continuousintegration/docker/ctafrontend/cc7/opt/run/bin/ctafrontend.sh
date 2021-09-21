#!/bin/bash

# @project        The CERN Tape Archive (CTA)
# @copyright      Copyright(C) 2021 CERN
# @license        This program is free software: you can redistribute it and/or modify
#                 it under the terms of the GNU General Public License as published by
#                 the Free Software Foundation, either version 3 of the License, or
#                 (at your option) any later version.
#
#                 This program is distributed in the hope that it will be useful,
#                 but WITHOUT ANY WARRANTY; without even the implied warranty of
#                 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#                 GNU General Public License for more details.
#
#                 You should have received a copy of the GNU General Public License
#                 along with this program.  If not, see <http://www.gnu.org/licenses/>.

. /opt/run/bin/init_pod.sh

if [ -e /cta_rpms ]; then
  echo 'Installing CTA RPMs'
  /usr/bin/yum install -y /cta_rpms/cta-*
fi

if [ -e /xroot_plugins ]; then
  echo 'Copying xroot conf'
  rm -rf /etc/cta/cta-frontend-xrootd.conf
  cp /xroot_plugins/cta-frontend-xrootd.conf /etc/cta/cta-frontend-xrootd.conf
fi

if [ ! -e /etc/buildtreeRunner ]; then
  # enable cta repository from previously built artifacts
  yum-config-manager --enable cta-artifacts
  yum-config-manager --enable ceph

  # install needed packages
  yum -y install cta-objectstore-tools mt-st mtx lsscsi sg3_utils cta-catalogueutils ceph-common oracle-instantclient19.3-sqlplus oracle-instantclient-tnsnames.ora
  yum clean packages
fi

echo "Using this configuration for library:"
/opt/run/bin/init_library.sh
cat /tmp/library-rc.sh
. /tmp/library-rc.sh

echo "Configuring objectstore:"
/opt/run/bin/init_objectstore.sh
. /tmp/objectstore-rc.sh

if [ "$KEEP_OBJECTSTORE" == "0" ]; then
  echo "Wiping objectstore"
  if [ "$OBJECTSTORETYPE" == "file" ]; then
    rm -fr $OBJECTSTOREURL
    mkdir -p $OBJECTSTOREURL
    cta-objectstore-initialize $OBJECTSTOREURL || die "ERROR: Could not Wipe the objectstore. cta-objectstore-initialize $OBJECTSTOREURL FAILED"
    chmod -R 777 $OBJECTSTOREURL
  else
    if [[ $(rados -p $OBJECTSTOREPOOL --id $OBJECTSTOREID --namespace $OBJECTSTORENAMESPACE ls | wc -l) -gt 0 ]]; then
      echo "Rados objectstore ${OBJECTSTOREURL} is not empty: deleting content"
      rados -p $OBJECTSTOREPOOL --id $OBJECTSTOREID --namespace $OBJECTSTORENAMESPACE ls | xargs -L 100 -P 100 rados -p $OBJECTSTOREPOOL --id $OBJECTSTOREID --namespace $OBJECTSTORENAMESPACE rm
    fi
    cta-objectstore-initialize $OBJECTSTOREURL || die "ERROR: Could not Wipe the objectstore. cta-objectstore-initialize $OBJECTSTOREURL FAILED"
    echo "Rados objectstore ${OBJECTSTOREURL} content:"
    rados -p $OBJECTSTOREPOOL --id $OBJECTSTOREID --namespace $OBJECTSTORENAMESPACE ls
  fi
else
  echo "Reusing objectstore (no check)"
fi

. /opt/run/bin/init_pod.sh

# oracle sqlplus client binary path
ORACLE_SQLPLUS="/usr/bin/sqlplus64"

die() {
  stdbuf -i 0 -o 0 -e 0 echo "$@"
  sleep 1
  exit 1
}

if [ ! -e /etc/buildtreeRunner ]; then
yum-config-manager --enable cta-artifacts
yum-config-manager --enable ceph

# Install missing RPMs
# cta-catalogueutils is needed to delete the db at the end of instance
yum -y install cta-frontend cta-debuginfo cta-catalogueutils ceph-common oracle-instantclient-tnsnames.ora
fi

# /etc/cta/cta-frontend-xrootd.conf is now provided by ctafrontend rpm. It comes with CI-ready content,
# except the objectstore backend path, which we add here:

/opt/run/bin/init_objectstore.sh
. /tmp/objectstore-rc.sh

ESCAPEDURL=$(echo ${OBJECTSTOREURL} | sed 's/\//\\\//g')
sed -i "s/^.*cta.objectstore.backendpath.*$/cta.objectstore.backendpath ${ESCAPEDURL}/" /etc/cta/cta-frontend-xrootd.conf

# Set the ObjectStore URL in the ObjectStore Tools configuration

echo "ObjectStore BackendPath $OBJECTSTOREURL" >/etc/cta/cta-objectstore-tools.conf

/opt/run/bin/init_database.sh
. /tmp/database-rc.sh

echo ${DATABASEURL} >/etc/cta/cta-catalogue.conf

# EOS INSTANCE NAME used as username for SSS key
EOSINSTANCE=ctaeos

# Wait for the keytab files to be pushed in by the creation script
echo -n "Waiting for /etc/cta/eos.sss.keytab."
for ((;;)); do test -e /etc/cta/eos.sss.keytab && break; sleep 1; echo -n .; done
echo OK
echo -n "Waiting for /etc/cta/cta-frontend.krb5.keytab."
for ((;;)); do test -e /etc/cta/cta-frontend.krb5.keytab && break; sleep 1; echo -n .; done
echo OK

echo "Core files are available as $(cat /proc/sys/kernel/core_pattern) so that those are available as artifacts"

# Configuring  grpc for cta-admin tapefile disk filename resolution
echo -n "Configuring grpc to ctaeos: "
if [ -r /etc/config/eoscta/eos.grpc.keytab ]; then
  cp /etc/config/eoscta/eos.grpc.keytab /etc/cta/eos.grpc.keytab
  echo 'cta.ns.config /etc/cta/eos.grpc.keytab' >> /etc/cta/cta-frontend-xrootd.conf
  echo 'OK'
else
  echo 'KO'
fi



if [ "-${CI_CONTEXT}-" == '-nosystemd-' ]; then
  # systemd is not available
  echo 'echo "Setting environment variables for cta-frontend"' > /tmp/cta-frontend_env
  cat /etc/sysconfig/cta-frontend | sed -e 's/^/export /' >> /tmp/cta-frontend_env
  source /tmp/cta-frontend_env

  # Not sure why /home/cta ends up ro but fix that
  mount -o remount,rw /home/cta

  # Copying xroot conf from mounted fs, symbolic link doesn't work
  if [ -e /xroot_plugins ]; then
    echo 'Copying xroot conf'
    rm -rf /etc/cta/cta-frontend-xrootd.conf
    cp /xroot_plugins/cta-frontend-xrootd.conf /etc/cta/cta-frontend-xrootd.conf
  fi

  runuser --shell='/bin/bash' --session-command='cd ~cta; xrootd -l /var/log/cta-frontend-xrootd.log -k fifo -n cta -c /etc/cta/cta-frontend-xrootd.conf -I v4' cta
  echo "ctafrontend died"
  echo "analysing core file if any"
  /opt/run/bin/ctafrontend_bt.sh
  sleep infinity
else
  # Add a DNS cache on the client as kubernetes DNS complains about `Nameserver limits were exceeded`
  yum install -y systemd-resolved
  systemctl start systemd-resolved

  # systemd is available
  echo "Launching frontend with systemd:"
  systemctl start cta-frontend

  echo "Status is now:"
  systemctl status cta-frontend
fi
