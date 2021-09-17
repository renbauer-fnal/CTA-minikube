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

fail() {
  echo $@ >&2
  exit 2
}

usage() {
  echo Starts the export of a tapepool to CTA.
  echo Fails if migrations are still ongoing. Migration routes are stored and removed from CASTOR.
  echo "Usage: $0 --dryrun|--doit tapepool [vo [eosctaInstance]]"
  exit 1
}

[ $# -ge 2 ] || usage

OPTION=$1
TAPEPOOL=$2
VO=${3:-ATLAS}
INSTANCE=${4:-eosctaatlas}

[ "$OPTION" = "--dryrun" -o "$OPTION" = "--doit" ] || usage

echo "Tapepool:${TAPEPOOL} VO:${VO} Instance:${INSTANCE} Option:${OPTION}"

# check arguments

# if NOT dry-run, execute all pre-checks and drop the migration routes
if [[ "$OPTION" == "--doit" ]]; then

  # check that the tapepool exists for this stager
  printtapepool ${TAPEPOOL} > /dev/null || fail 'Tape pool' ${TAPEPOOL} 'not found or not configured on this stager'

  # check that no migrations are pending/ongoing for this tapepool
  mig=`printmigrationstatus | grep -c ${TAPEPOOL}`
  [[ $mig -gt 0 ]] && fail 'Migrations still ongoing, aborting'

  # check that all tapes are good for export, that is no BUSY tape; RDONLY is OK
  busytapes=`vmgrlisttape -P ${TAPEPOOL} | grep -c BUSY`
  [[ $busytapes -gt 0 ]] && fail 'Found' $busytapes 'tape(s) in BUSY state, aborting'

  # on the stager, make the tapepool unusable (the tapepool metadata can stay)
  echo "No ongoing tape migration found, backing up and removing migration routes"
  mkdir -p ~/ctaexport
  [[ ! -x ~/ctaexport/migrationroutes_${TAPEPOOL} ]] && printmigrationroute | grep -w ${TAPEPOOL} > ~/ctaexport/migrationroutes_${TAPEPOOL}
  printmigrationroute | grep -w ${TAPEPOOL} | awk '{print $1}' | xargs -i deletemigrationroute {}

  # all right, still warn user about repack
  echo "Moving on ASSUMING no repack operation has been submitted concerning tapepool" ${TAPEPOOL}
fi

# from now on, exit for any error from any command
set -e

# execute the DB extraction from the CTA DB
tapepool_castor_to_cta.py -t ${TAPEPOOL} -v ${VO} -i ${INSTANCE} ${OPTION}
[ $? -eq 0 ] || fail "Exited during tapepool export"

# execute the EOS metadata import
eos-import-dirs --delta /   # should there be any
[ $? -eq 0 ] || fail "Exited during directory import"

eos-import-files
[ $? -eq 0 ] || fail "Exited during file import"

# Report any intermediate failures
eos-import-dirs -f
eos-import-files -f

# terminate the export
complete_tapepool_export.py -t ${TAPEPOOL}
