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

VID=$1

INCREMENT=3

STDEVICE=`/bin/readlink --canonicalize /dev/tape | /bin/sed -e 's;^/dev/nst;/dev/st;'`
SGDEVICE=`/usr/bin/lsscsi --generic | /bin/grep tape | /bin/grep $STDEVICE | /bin/awk '{print $7}'`

date
mt rewind
mt stat | grep IM_REP_EN

FSEQ=1
NOT_AT_EOD=1
while [ $NOT_AT_EOD -eq 1 ]
do
  RAWDATA=`/usr/bin/sg_requests --hex $SGDEVICE | /usr/bin/cut -c9- | /usr/bin/sed ':a;N;$!ba;s/\n/ /g;s/ \+/ /g'`
  WRAP=`echo $RAWDATA | /bin/awk -Wposix '{ printf("%d", "0x"$30) }'` # 29 + 1
  WRAPLPOS=`echo $RAWDATA | /bin/awk -Wposix '{ printf("%d", "0x"$31$32$33$34) }'` # 30-33 - each + 1
  LOGICALBLOCKDEC=`echo $RAWDATA | /bin/awk -Wposix '{ printf("%d", "0x"$46$47$48$49) }'` # 45-48 - each + 1
  LOGICALBLOCKHEX=`echo $RAWDATA | /bin/awk -Wposix '{ print $46$47$48$49 }'` # 45-48 - each + 1
  echo "VID: $VID, NSFSEQ: $FSEQ, WRAP: $WRAP, WRAP LPOS: $WRAPLPOS, LOGICAL BLOCK (DEC): $LOGICALBLOCKDEC, LOGICAL BLOCK (HEX): $LOGICALBLOCKHEX"

  mt fsf $INCREMENT

  AT_EOD=`mt stat | grep -c EOD`
  if [ $AT_EOD -eq 1 ]
  then
    RAWDATA=`/usr/bin/sg_requests --hex $SGDEVICE | /usr/bin/cut -c9- | /usr/bin/sed ':a;N;$!ba;s/\n/ /g;s/ \+/ /g'`
    WRAP=`echo $RAWDATA | /bin/awk -Wposix '{ printf("%d", "0x"$30) }'` # 29 + 1
    WRAPLPOS=`echo $RAWDATA | /bin/awk -Wposix '{ printf("%d", "0x"$31$32$33$34) }'` # 30-33 - each + 1
    LOGICALBLOCKDEC=`echo $RAWDATA | /bin/awk -Wposix '{ printf("%d", "0x"$46$47$48$49) }'` # 45-48 - each + 1
    LOGICALBLOCKHEX=`echo $RAWDATA | /bin/awk -Wposix '{ print $46$47$48$49 }'` # 45-48 - each + 1
    echo "VID: $VID, NSFSEQ: $FSEQ, WRAP: $WRAP, WRAP LPOS: $WRAPLPOS, LOGICAL BLOCK (DEC): $LOGICALBLOCKDEC, LOGICAL BLOCK (HEX): $LOGICALBLOCKHEX, EOD"
    NOT_AT_EOD=0
  fi

  let FSEQ=$FSEQ+1
done

mt stat | grep IM_REP_EN
date
