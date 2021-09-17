#!/bin/sh

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

# Initialise CTA Catalogue on first use

# LHC experiments
cta-admin virtualorganization add --vo "ALICE"   --comment "ALICE"    --readmaxdrives 2 --writemaxdrives 2
cta-admin virtualorganization add --vo "ATLAS"   --comment "ATLAS"    --readmaxdrives 2 --writemaxdrives 2
cta-admin virtualorganization add --vo "CMS"     --comment "CMS"      --readmaxdrives 2 --writemaxdrives 2
cta-admin virtualorganization add --vo "LHCb"    --comment "LHCb"     --readmaxdrives 2 --writemaxdrives 2

# SME experiments on Namespace dashboard
cta-admin virtualorganization add --vo "COMPASS" --comment "COMPASS"  --readmaxdrives 2 --writemaxdrives 2
cta-admin virtualorganization add --vo "NA64"    --comment "NA64"     --readmaxdrives 2 --writemaxdrives 2
cta-admin virtualorganization add --vo "AMS"     --comment "AMS"      --readmaxdrives 2 --writemaxdrives 2
cta-admin virtualorganization add --vo "nTOF"    --comment "nTOF"     --readmaxdrives 2 --writemaxdrives 2

# Other SME experiments
cta-admin virtualorganization add --vo "NA62"    --comment "NA62"     --readmaxdrives 2 --writemaxdrives 2
cta-admin virtualorganization add --vo "COMPASS" --comment "COMPASS"  --readmaxdrives 2 --writemaxdrives 2

# Media types as discovered from CASTOR (17/06/2020)
#   SQL> select unique density from vmgr_tape_info;
#
#   DENSITY
#   --------
#   20TC
#   15TC
#   9TC
#   12TC
#   7000GC

cta-admin mediatype add \
    --name T10K500G  \
    --capacity 500000000000 \
    --primarydensitycode 74 \
    --cartridge "T10000" \
    --comment "Oracle T10000 cartridge formated at 500 GB (for developers only)"
cta-admin mediatype add \
    --name 3592JC7T \
    --capacity 7000000000000 \
    --primarydensitycode 84 \
    --cartridge "3592JC" \
    --comment "IBM 3592JC cartridge formated at 7 TB"
cta-admin mediatype add \
    --name 3592JD15T \
    --capacity 15000000000000 \
    --primarydensitycode 85 \
    --cartridge "3592JD" \
    --comment "IBM 3592JD cartridge formated at 15 TB"
cta-admin mediatype add \
    --name 3592JE20T \
    --capacity 20000000000000 \
    --primarydensitycode 87 \
    --cartridge "3592JE" \
    --comment "IBM 3592JE cartridge formated at 20 TB"
cta-admin mediatype add \
    --name LTO7M \
    --capacity 9000000000000 \
    --primarydensitycode 93 \
    --cartridge "LTO-7" \
    --comment "LTO-7 M8 cartridge formated at 9 TB"
cta-admin mediatype add \
    --name LTO8 \
    --capacity 12000000000000 \
    --primarydensitycode 94 \
    --cartridge "LTO-8" \
    --comment "LTO-8 cartridge formated at 12 TB"
