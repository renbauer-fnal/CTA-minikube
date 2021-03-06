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

usage() { cat <<EOF 1>&2
Usage: $0 -n <namespace>
EOF
exit 1
}

while getopts "n:" o; do
    case "${o}" in
        n)
            NAMESPACE=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if [ -z "${NAMESPACE}" ]; then
    usage
fi

if [ ! -z "${error}" ]; then
    echo -e "ERROR:\n${error}"
    exit 1
fi

TMPDIR=$(mktemp -d --suffix .testflight)

FLIGHTTEST_RC=0 # flighttest return code

###
# TPC capabilities
#
# if the installed xrootd version is not the same as the one used to compile eos
# Third Party Copy can be broken
#
# Valid check:
# [root@eosctafst0017 ~]# xrdfs
# root://pps-ngtztag6p5ht-minion-2.cern.ch:1101 query config tpc
# 1
#
# invalid check:
# [root@ctaeos /]# xrdfs root://ctaeos.toto.svc.cluster.local:1095 query config tpc
# tpc

TEST_LOG="${TMPDIR}/tpc_cap.log"
echo "Checking xrootd TPC capabilities on FSTs:"
kubectl -n ${NAMESPACE} exec ctaeos -- eos node ls -m | grep status=online | sed -e 's/.*hostport=//;s/ .*//' | xargs -ifoo bash -o pipefail -c "kubectl -n ${NAMESPACE} exec ctaeos -- xrdfs root://foo query config tpc | grep -q 1 && echo foo: OK|| echo foo: KO" | tee ${TEST_LOG}

if $(cat ${TEST_LOG} | grep -q KO); then
  echo "TPC capabilities: FAILED"
  ((FLIGHTTEST_RC+=1))
else
  echo "TPC capabilities: SUCCESS"
fi
echo

rm -fr ${TMPDIR}
exit ${FLIGHTTEST_RC}
