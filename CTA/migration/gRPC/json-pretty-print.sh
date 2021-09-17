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

# Pretty Print JSON with Base64 decoding of byte-encoded fields

if [ $# -eq 1 ]
then
  cat $1
else
  cat -
fi | sed 's/\]\[/,/g' | python -mjson.tool |\
awk '/"path": / || /"name": / ||
     /"value": / ||
     /"CTA_.*": / ||
     /"sys\..*": / ||
     /"eos.btime": / {
       CODE=substr($2, 2)
       sub(/",*$/,"",CODE)
       command="echo "CODE"|base64 -d"
       command | getline DECODED
       close(command)
       sub(CODE,DECODED)
   }
   { print $0 }'
