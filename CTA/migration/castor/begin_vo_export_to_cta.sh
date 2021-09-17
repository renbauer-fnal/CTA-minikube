#!/bin/bash

# @project        The CERN Tape Archive (CTA)
# @copyright      Copyright(C) 2019-2021 CERN
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

# exit for any error from any command
set -e

print_usage() {
  echo Starts the export to CTA and blocks access to CASTOR.
  echo Usage: $0 --dryrun\|--doit dir1 dir2 ...
  echo '       --dryrun is dry-run mode for CASTOR. With --doit, the CASTOR directories mode bits are reset to 000.'
  echo '       dir1 dir2 ...  list of CASTOR top-level directories without prefix, e.g. /user'
  exit 1
}

# parse arguments
if [[ $# -eq 0 ]]; then
  print_usage
fi
while [[ $# -gt 0 ]]; do
  arg=$1
  case $arg in
    --dryrun) doit=0; shift;;
    --doit) doit=1; shift;;
    *) break;;
  esac
done
if [[ "$@" == "" ]]; then
  print_usage
fi


mkdir -p ~/ctaexport

for tld in $@; do
  # if NOT dry-run, block access in the namespace to the given top-level dir
  if [[ "$doit" == "1" ]]; then
    tldnoslash=`echo ${tld} | sed "s|\/|_|g"`
    nsls -ld /castor/cern.ch/$tld > ~/ctaexport/nsls${tldnoslash}
    nsgetacl /castor/cern.ch/$tld > ~/ctaexport/nsgetacl${tldnoslash}
    nschmod 000 /castor/cern.ch/$tld
    echo `date +%Y-%m-%dT%H:%M:%S`'  Stored ACLs and blocked access to' $tld
  fi

  # execute the DB extraction and EOS metadata import for the top-level dir
  echo `date +%Y-%m-%dT%H:%M:%S`'  Importing CASTOR tree below' $tld
  eos-import-dirs $tld
done
echo `date +%Y-%m-%dT%H:%M:%S`'  CASTOR directories import completed successfully'

# empty the CASTOR disk cache (not necessary)
#for h in `printdiskserver | grep cern.ch | awk '{print $1}'`; do
# for f in `seq -w 1 24`; do
#   deletediskcopy $h:/srv/castor/$f/
# done
#done
