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

set -e

DEVTOOLSET_ENABLE_SCRIPT=/opt/rh/devtoolset-8/enable
EOS_BUILD_DIR=~/eos_build
EOS_SRC_DIR=~/eos

echo "DEVTOOLSET_ENABLE_SCRIPT=${DEVTOOLSET_ENABLE_SCRIPT}"
if ! test -f ${DEVTOOLSET_ENABLE_SCRIPT}; then
  echo "The devtoolset enable script ${DEVTOOLSET_ENABLE_SCRIPT} does not exists or is not a regular file"
  exit 1
fi

echo "EOS_SRC_DIR=${EOS_SRC_DIR}"
if ! test -d ${EOS_SRC_DIR}; then
  echo "The ${EOS_SRC_DIR} directory does not exists or is not a directory"
  exit 1
fi

echo "Sourcing ${DEVTOOLSET_ENABLE_SCRIPT}"
. ${DEVTOOLSET_ENABLE_SCRIPT}

echo "Deleting ${EOS_BUILD_DIR}"
rm -rf ${EOS_BUILD_DIR}

echo "Creating ${EOS_BUILD_DIR}"
mkdir -p ${EOS_BUILD_DIR}

cd ${EOS_BUILD_DIR}
cmake3 -DPACKAGEONLY=1 ${EOS_SRC_DIR}
make srpm

sudo yum-builddep -y ${EOS_BUILD_DIR}/SRPMS/eos-*.cern.src.rpm
