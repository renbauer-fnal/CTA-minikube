/*!
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2020-2021 CERN
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

-- Remove Procedures
DROP PROCEDURE importTapePool;
DROP PROCEDURE populateCTAFilesFromCASTOR;
DROP PROCEDURE importFromCASTOR;
DROP PROCEDURE removeCASTORMetadata;

-- Remove Types
DROP TYPE NUMLIST;

-- Remove Synonyms
DROP SYNONYM CNS_CTAFilesHelper;
DROP SYNONYM CNS_CTAFiles2ndCopyHelper;
DROP SYNONYM CNS_CTADirsHelper;
DROP SYNONYM CNS_CTAMigrationLog;
DROP SYNONYM CNS_Class_Metadata;
DROP SYNONYM CNS_Dirs_Full_Path;
DROP SYNONYM CNS_filesForCTAExport;
DROP SYNONYM CNS_zeroByteFilesForCTAExport;
DROP SYNONYM CNS_dirsForCTAExport;
DROP SYNONYM CNS_ctaLog;
DROP SYNONYM CNS_getTime;
DROP SYNONYM Vmgr_tape_side;
DROP SYNONYM vmgr_tape_info;
DROP SYNONYM Vmgr_tape_dgnmap;

-- Remove temporary table
DROP TABLE Temp_Remove_CASTOR_Metadata;

-- Remove Error Log created with dbms_errlog.create_error_log(dml_table_name => 'Tape_File');
DROP TABLE ERR$_TAPE_FILE;

-- Remove PARALLEL
ALTER TABLE ARCHIVE_FILE NOPARALLEL;
ALTER TABLE TAPE_FILE NOPARALLEL;

QUIT
