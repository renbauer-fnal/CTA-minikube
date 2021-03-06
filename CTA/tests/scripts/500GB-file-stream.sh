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

TESTFILENAME=/var/tmp/cta-test-temporary-file-$$
EOSDIR=/eos/dev/test/vlado/`date +%Y%m%d%H%M`

export XRD_STREAMTIMEOUT=600     # increased from 60s
export XRD_TIMEOUTRESOLUTION=600 # increased from 15s

# Usage
if ! [[ "0" -lt "$1" ]]
then
  echo "Usage: $0 <filesize in bytes>"
  exit -1
fi

TESTFILESIZE=$1

# Generate random file (make smaller (< 1 GB) files ASCII hence compressible (size not accurate though))
if [ "$TESTFILESIZE" -lt "1000000000" ]
then
  openssl rand $TESTFILESIZE -base64 -out $TESTFILENAME
else
  openssl rand $TESTFILESIZE -out $TESTFILENAME
fi

# Compute how many of these files would be needed to fill 100 GB
let FILESCOUNT=100000000000/$TESTFILESIZE

echo "EOS directory: $EOSDIR, File size: $SIZE Files count: $FILESCOUNT"

for i in `seq 1 $FILESCOUNT`
do
  if [ "$TESTFILESIZE" -lt "1000000000" ]
  then
    # Make smaller (< 1 GB) source files unique in order to excercise different checksums
    echo $i >> $TESTFILENAME
  fi
  xrdcp $TESTFILENAME root://eosctatape//$EOSDIR/file_$i && echo "File $EOSDIR/file_$i copied at "`date "+%Y-%m-%d %H:%M:%S"`
done

# Cleanup
rm -f $TESTFILENAME
