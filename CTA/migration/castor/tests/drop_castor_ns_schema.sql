/*!
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2021 CERN
 * @license        This program is free software: you can redistribute it and/or modify
 *                 it under the terms of the GNU General Public License as published by
 *                 the Free Software Foundation, either version 3 of the License, or
 *                 (at your option) any later version.
 *
 *                 This program is distributed in the hope that it will be useful,
 *                 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *                 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *                 GNU General Public License for more details.
 *
 *                 You should have received a copy of the GNU General Public License
 *                 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

drop table CASTORCONFIG;
drop table CNS_TP_POOL;
drop table CNS_USER_METADATA;
drop table CNS_SYMLINKS;
drop table CNS_FILES_EXIST_TMP;
drop table CNS_SEG_METADATA;
drop table CNS_FILE_METADATA;
drop table CNS_CLASS_METADATA;
drop table SetSegsForFilesInputHelper;
drop table SetSegsForFilesResultsHelper;
drop table Dirs_Full_Path;

drop sequence Cns_unique_id;
drop function getTime;
drop procedure cns_ctaLog;
