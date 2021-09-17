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

install(FILES
    ${CMAKE_SOURCE_DIR}/migration/castor/begin_vo_export_to_cta.sh
    ${CMAKE_SOURCE_DIR}/migration/castor/export_production_tapepool_to_cta.sh
    ${CMAKE_SOURCE_DIR}/migration/castor/tapepool_castor_to_cta.py
    ${CMAKE_SOURCE_DIR}/migration/castor/zerolen_castor_to_cta.py
    ${CMAKE_SOURCE_DIR}/migration/castor/complete_cta_export.py
    ${CMAKE_SOURCE_DIR}/migration/castor/vmgr_reenable_tapepool.sh
    DESTINATION usr/bin)
