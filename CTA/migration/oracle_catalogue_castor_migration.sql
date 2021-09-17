/*!
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2019-2021 CERN
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

-- whenever sqlerror exit sql.sqlcode;
-- Create synonyms for all relevant entities
UNDEF castornsSchema
ACCEPT castornsSchema CHAR PROMPT 'Enter the name of the CASTOR Nameserver schema: ';
CREATE OR REPLACE SYNONYM CNS_CTAFilesHelper FOR &castornsSchema..CTAFilesHelper;
CREATE OR REPLACE SYNONYM CNS_CTAFiles2ndCopyHelper FOR &castornsSchema..CTAFiles2ndCopyHelper;
CREATE OR REPLACE SYNONYM CNS_CTADirsHelper FOR &castornsSchema..CTADirsHelper;
CREATE OR REPLACE SYNONYM CNS_CTAMigrationLog FOR &castornsSchema..CTAMigrationLog;
CREATE OR REPLACE SYNONYM CNS_Class_Metadata FOR &castornsSchema..Cns_class_metadata;
CREATE OR REPLACE SYNONYM CNS_Dirs_Full_Path FOR &castornsSchema..Dirs_Full_Path;

CREATE OR REPLACE SYNONYM CNS_filesForCTAExport FOR &castornsSchema..filesForCTAExport;
CREATE OR REPLACE SYNONYM CNS_zeroByteFilesForCTAExport FOR &castornsSchema..zeroByteFilesForCTAExport;
CREATE OR REPLACE SYNONYM CNS_dirsForCTAExport FOR &castornsSchema..dirsForCTAExport;
CREATE OR REPLACE SYNONYM CNS_ctaLog FOR &castornsSchema..ctaLog;
CREATE OR REPLACE SYNONYM CNS_getTime FOR &castornsSchema..getTime;

UNDEF vmgrSchema
ACCEPT vmgrSchema CHAR PROMPT 'Enter the name of the VMGR schema: ';
CREATE OR REPLACE SYNONYM Vmgr_tape_side FOR &vmgrSchema..Vmgr_tape_side;
CREATE OR REPLACE SYNONYM vmgr_tape_info FOR &vmgrSchema..Vmgr_tape_info;
CREATE OR REPLACE SYNONYM Vmgr_tape_dgnmap FOR &vmgrSchema..Vmgr_tape_dgnmap;

-- Used by removeCASTORMetadata
CREATE OR REPLACE TYPE NUMLIST IS TABLE OF INTEGER;
/

-- Drop temporary table if it exists before recreating it
BEGIN
EXECUTE IMMEDIATE 'DROP TABLE Temp_Remove_CASTOR_Metadata';
EXCEPTION
WHEN OTHERS THEN NULL;
END;
/

CREATE GLOBAL TEMPORARY TABLE Temp_Remove_CASTOR_Metadata (archive_file_id INTEGER PRIMARY KEY)
ON COMMIT DELETE ROWS;

-- Import a tapepool and its tapes from CASTOR
-- Raises constraint_violation if the tapepool and/or some tapes were already imported
CREATE OR REPLACE PROCEDURE importTapePool(inTapePool VARCHAR2, inVO_id INTEGER) AS
  varUnused VARCHAR2(100);
  CONSTRAINT_VIOLATED EXCEPTION;
  PRAGMA EXCEPTION_INIT(CONSTRAINT_VIOLATED, -1);
  varTapePoolId Tape_Pool.tape_pool_id%TYPE := 0;
BEGIN
  FOR DGN IN (SELECT dgn FROM Vmgr_tape_dgnmap) LOOP
    -- The very first time insert the logical libraries found on VMGR
    BEGIN
      INSERT INTO Logical_Library (logical_library_id, logical_library_name, user_comment,
        creation_log_user_name, creation_log_host_name, creation_log_time,
        last_update_user_name, last_update_host_name, last_update_time)
      VALUES (
        Logical_Library_id_seq.NEXTVAL,
        DGN.dgn,
        'Imported from CASTOR',
        'CASTOR', 'CASTOR', CNS_getTime(),
        'CASTOR', 'CASTOR', CNS_getTime()
        );
    EXCEPTION WHEN CONSTRAINT_VIOLATED THEN
      NULL;   -- it was already present, skip
    END;
  END LOOP;

  BEGIN
    SELECT Tape_Pool_id_seq.NEXTVAL INTO varTapePoolId FROM Dual;
    INSERT INTO Tape_Pool (tape_pool_id, tape_pool_name, virtual_organization_id, nb_partial_tapes, is_encrypted, user_comment,
      creation_log_user_name, creation_log_host_name, creation_log_time, last_update_user_name,
      last_update_host_name, last_update_time)
    VALUES (
      varTapePoolId,
      inTapePool,
      inVO_id,
      0,    -- nb_partial_tapes, to be filled afterwards
      '0',  -- is_encrypted is assumed false in CASTOR
      'Imported from CASTOR',
      'CASTOR', 'CASTOR', CNS_getTime(),
      'CASTOR', 'CASTOR', CNS_getTime()
      );
  EXCEPTION WHEN CONSTRAINT_VIOLATED THEN
    -- The TapePool is already present, typically because of a previous import: override some values
    UPDATE Tape_Pool SET
           virtual_organization_id = inVO_id,
           user_comment = 'Re-imported from CASTOR',
           last_update_user_name = 'CASTOR',
           last_update_host_name = 'CASTOR',
           last_update_time = CNS_getTime()
     WHERE tape_pool_name = inTapePool
    RETURNING tape_pool_id INTO varTapePoolId;
  END;
  FOR T in (SELECT TI.vid, TI.density, TI.manufacturer, DGN.dgn, TS.status, TS.nbfiles,
                   TI.rcount, TI.wcount, TI.rhost, TI.whost, TI.rtime, TI.wtime
              FROM Vmgr_tape_info TI, Vmgr_tape_side TS, Vmgr_tape_dgnmap DGN
             WHERE TI.vid = TS.vid
               AND TI.library = DGN.library
               AND TI.model = DGN.model
               AND BITAND(TS.status, 2) = 0 AND BITAND(TS.status, 32) = 0   -- not already EXPORTED or ARCHIVED
               AND TS.poolname = inTapePool) LOOP
    INSERT INTO Tape (vid, media_type_id, vendor, logical_library_id, tape_pool_id,
      encryption_key_name, data_in_bytes, last_fseq,
      is_full, is_from_castor,
      label_drive, label_time, last_read_drive, last_read_time, read_mount_count,
      last_write_drive, last_write_time, write_mount_count,
      user_comment, tape_state, state_reason, state_update_time, state_modified_by,
      creation_log_user_name, creation_log_host_name, creation_log_time,
      last_update_user_name, last_update_host_name, last_update_time)
    VALUES (
      T.vid,
      (SELECT MEDIA_TYPE_ID FROM MEDIA_TYPE WHERE MEDIA_TYPE_NAME = decode(T.density,
        -- media type: only one of the options below (see #488)
        '7000GC', '3592JC7T',
        '9TC',    'LTO7M',
        '12TC',   'LTO8',
        '15TC',   '3592JD15T',
        '20TC',   '3592JE20T',
        'UNDEFINED')),
      T.manufacturer,
      (SELECT logical_library_id FROM Logical_Library WHERE logical_library_name = T.dgn),
      varTapePoolId,
      NULL,                -- empty encryption key name
      0,      -- total data: will be filled by populateCTAFilesFromCASTOR()
      T.nbfiles,
      -- decode(BITAND(T.status, 1), 1, '1', '0'),    -- DISABLED flag
      decode(BITAND(T.status, 8), 8, '1', '0'),    -- FULL flag
      -- decode(BITAND(T.status, 16), 16, '1', '0'),  -- RDONLY flag
      '1',                                         -- is_from_castor flag
      'CASTOR', 0,                  -- label drive and time (unknown)
      T.rhost, T.rtime, T.rcount,   -- last read drive/time and count
      T.whost, T.wtime, T.wcount,   -- last write drive/time and count
      'Imported from CASTOR',
      (case when BITAND(T.status, 1) = 1 then 'DISABLED' else 'ACTIVE' end), -- tape_state
      'Imported from CASTOR', -- state_reason
      CNS_getTime(), -- state_update_time
      'importTapePool@castor', -- state_modified_by
      'CASTOR', 'CASTOR', CNS_getTime(),
      'CASTOR', 'CASTOR', CNS_getTime()
      );
  END LOOP;
  COMMIT;
  CNS_ctaLog(inTapePool, 'Tapes import completed');
END;
/


-- Insert the file-level metadata for the given migration
CREATE OR REPLACE PROCEDURE populateCTAFilesFromCASTOR(inEOSCTAInstance VARCHAR2, inTapePool VARCHAR2, inVO_id INTEGER) AS
  CONSTRAINT_VIOLATED EXCEPTION;
  PRAGMA EXCEPTION_INIT(CONSTRAINT_VIOLATED, -1);
  varIs2ndCopy INTEGER;
  nbPreviousErrors INTEGER;
  nbMissingImports INTEGER;
BEGIN
  -- Populate storage classes metadata if missing: do one by one and handle constraint violations
  FOR c IN (SELECT classid, name classname, nbcopies
              FROM CNS_Class_Metadata
             WHERE classid IN
              (SELECT classid FROM CNS_CTAFilesHelper)) LOOP
    BEGIN
      INSERT INTO Storage_Class (storage_class_id, storage_class_name, nb_copies,
        virtual_organization_id, user_comment,
        creation_log_user_name, creation_log_host_name, creation_log_time,
        last_update_user_name, last_update_host_name, last_update_time)
      VALUES (
        c.classid,
        c.classname,
        c.nbcopies,
        inVO_id,
        'Imported from CASTOR',
        'CASTOR', 'CASTOR', CNS_getTime(),
        'CASTOR', 'CASTOR', CNS_getTime()
        );
    EXCEPTION WHEN CONSTRAINT_VIOLATED THEN
      UPDATE Storage_Class SET     -- it is already present, update it with the current values
             storage_class_name = c.classname,
             nb_copies = c.nbcopies,
             virtual_organization_id = inVO_id,
             user_comment = 'Re-imported from CASTOR',
             last_update_user_name = 'CASTOR',
             last_update_host_name = 'CASTOR',
             last_update_time = CNS_getTime()
       WHERE storage_class_id = c.classid;
    END;
  END LOOP;
  COMMIT;
  CNS_ctaLog(inTapePool, 'Storage classes import completed');

  -- Populate the CTA catalogue with the CASTOR file metadata: this runs in a single transaction
  INSERT INTO Archive_File
    (archive_file_id, disk_instance_name, disk_file_id,
    disk_file_uid, disk_file_gid, size_in_bytes, checksum_adler32,
    storage_class_id, creation_time, reconciliation_time, is_deleted) (
    SELECT F.fileid, inEOSCTAInstance, F.fileid, F.disk_uid, F.disk_gid, F.filesize, F.checksum,
           F.classid, F.btime, 0, '0'
      FROM CNS_CTAFilesHelper F
           -- no need to exclude already existing files because of dual copies
           -- as this is validated before hand in the cns_filesForCTAExport procedure
    );
  CNS_ctaLog(inTapePool, 'Archive files import completed');

  -- Import the tape file metadata: here we also store the constraint_violation
  -- errors for later manual analysis
  SELECT COUNT(*) INTO nbPreviousErrors FROM Err$_Tape_File;
  INSERT INTO Tape_File
    (archive_file_id, logical_size_in_bytes,
     vid, fseq, block_id, copy_nb, creation_time) (
    SELECT fileid, filesize, vid, fseq, blockId, copyno, s_mtime FROM CNS_CTAFilesHelper F
     WHERE filesize > 0
    ) LOG ERRORS INTO Err$_Tape_File ('Importing '|| inTapePool) REJECT LIMIT UNLIMITED;
  -- Check if we're dealing with dual copy files
  SELECT COUNT(*) INTO varIs2ndCopy FROM Dual
   WHERE EXISTS (SELECT 1 FROM CNS_CTAFiles2ndCopyHelper);
  IF varIs2ndCopy = 1 THEN
    -- Yes, do the same import for the 2nd copies
    INSERT INTO Tape_File
      (archive_file_id, logical_size_in_bytes,
      vid, fseq, block_id, copy_nb, creation_time) (
      SELECT fileid, filesize, vid, fseq, blockId, 2, s_mtime FROM CNS_CTAFiles2ndCopyHelper F
      ) LOG ERRORS INTO Err$_Tape_File ('Importing '|| inTapePool) REJECT LIMIT UNLIMITED;
  END IF;
  SELECT COUNT(*) INTO nbMissingImports FROM Err$_Tape_File;
  IF nbMissingImports = nbPreviousErrors THEN
    IF nbPreviousErrors > 0 THEN
      CNS_ctaLog(inTapePool, 'Tape files import completed, but there are still '|| nbPreviousErrors
                             ||' problematic cases from previous imports in the Err$_Tape_File table');
    ELSE
      CNS_ctaLog(inTapePool, 'Tape files import completed');
    END IF;
  ELSE
    CNS_ctaLog(inTapePool, 'Tape files import NOT fully completed, '|| (nbMissingImports-nbPreviousErrors)
                           ||' files are missing: please check the Err$_Tape_File table');
  END IF;

  -- Update data counters on the Tape table: this does NOT include deleted segments,
  -- but it's the best approximation we have here
  FOR v IN (SELECT vid, sum(filesize) totalpervid FROM CNS_CTAFilesHelper GROUP BY vid
            UNION ALL
            SELECT vid, sum(filesize) totalpervid FROM CNS_CTAFiles2ndCopyHelper GROUP BY vid) LOOP
    UPDATE Tape
       SET dirty = 1, data_in_bytes = data_in_bytes + v.totalpervid
     WHERE vid = v.vid;
  END LOOP;
  COMMIT;
  CNS_ctaLog(inTapePool, 'Updated Tape data counters');
END;
/


-- Entry point to import metadata from the CASTOR namespace
CREATE OR REPLACE PROCEDURE importFromCASTOR(inTapePool VARCHAR2, inVO VARCHAR2, inEOSCTAInstance VARCHAR2, inZeroBytes IN INTEGER, inDryRun INTEGER) AS
  nbFiles INTEGER;
  inVO_id INTEGER;
  varTapePool VARCHAR2(100) := inTapePool;
  var2ndTapePool VARCHAR2(100) := NULL;
BEGIN
  -- First check if there's anything already ongoing and fail early:
  SELECT COUNT(*) INTO nbFiles FROM CNS_CTAFilesHelper;
  IF nbFiles > 0 THEN
    raise_application_error(-20000, 'Another export of ' || nbFiles || ' files to CTA is ongoing, ' ||
                            'please terminate it with complete_tapepool_export.py before starting a new one.');
  END IF;
  IF inZeroBytes = 1 THEN
    varTapePool := upper(inVO) || '::*';    -- made-up convention for logging purposes
  END IF;

  -- get Virtual Organization ID (VO table must be pre-filled using 'cta-admin vo add ...')
  BEGIN
    SELECT VIRTUAL_ORGANIZATION_ID INTO inVO_id FROM VIRTUAL_ORGANIZATION WHERE VIRTUAL_ORGANIZATION_NAME = inVO;
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      raise_application_error(-20000, 'Virtual organization ' || inVO || ' not found in VIRTUAL_ORGANIZATION table.');
  END;

  IF inDryRun = 0 THEN
    CNS_ctaLog(varTapePool, 'CASTOR metadata import started');
  ELSE
    CNS_ctaLog(varTapePool, 'CASTOR metadata import started [dry-run mode]');
  END IF;
  BEGIN
    IF inZeroBytes = 0 THEN
      -- extract all relevant files metadata and work out if dual tape copies are to be imported; can raise exceptions
      CNS_filesForCTAExport(inTapePool, var2ndTapePool);
      -- extract all directories metadata
      CNS_dirsForCTAExport(inTapePool);
      -- import tapes; can raise exceptions
      importTapePool(inTapePool, inVO_id);
      IF var2ndTapePool IS NOT NULL THEN
        importTapePool(var2ndTapePool, inVO_id);
      END IF;
      -- import metadata into the CTA catalogue
      populateCTAFilesFromCASTOR(inEOSCTAInstance, varTapePool, inVO_id);
      IF inDryRun = 0 THEN
        CNS_ctaLog(varTapePool, 'CASTOR metadata import completed successfully');
      ELSE
        CNS_ctaLog(varTapePool, 'CASTOR metadata import completed successfully [dry-run mode]');
      END IF;
    ELSE
      -- 0-byte files are special as they do not belong to any tape pool
      CNS_zeroByteFilesForCTAExport(inVO);
      -- extract all directories metadata as above
      CNS_dirsForCTAExport(varTapePool);
      CNS_ctaLog(varTapePool, 'CASTOR zero-length file metadata imported successfully');
    END IF;
  EXCEPTION WHEN OTHERS THEN
    -- any error is logged and raised to the caller
    CNS_ctaLog(varTapePool, 'Exception caught, terminating import: '|| SQLERRM ||' '|| dbms_utility.format_error_backtrace());
    RAISE;
  END;
END;
/


-- Entry point to remove the CASTOR imported metadata from the CTA catalogue
CREATE OR REPLACE PROCEDURE removeCASTORMetadata(inTapePool VARCHAR2) AS
  varTapePoolId INTEGER;
  varCount INTEGER;
  varIs2ndCopy INTEGER;
  var2ndTapePoolId INTEGER := 0;
  var2ndTapePool VARCHAR2(100);
  CURSOR temp_data_cur IS SELECT archive_file_id FROM Temp_Remove_CASTOR_Metadata;
  ids numList;
  varTempId INTEGER;
BEGIN
  BEGIN
    SELECT tape_pool_id INTO varTapePoolId
      FROM Tape_Pool
     WHERE tape_pool_name = inTapePool;
  EXCEPTION WHEN NO_DATA_FOUND THEN
    raise_application_error(-20000, 'No such tape pool found');
  END;
  -- prepare the list of files to be removed in a temporary table
  INSERT INTO Temp_Remove_CASTOR_Metadata T
    (SELECT archive_file_id FROM Tape_File TF WHERE vid IN
      (SELECT vid FROM Tape WHERE tape_pool_id = varTapePoolId AND is_from_castor = '1'));
  SELECT COUNT(*) INTO varCount FROM Temp_Remove_CASTOR_Metadata;
  CNS_ctaLog(inTapePool, 'Removal of CASTOR files and tapes metadata from CTA started, '|| varCount ||' files to go');
  -- check for dual copy case
  SELECT COUNT(*) INTO varIs2ndCopy FROM Dual
  WHERE EXISTS (
    SELECT 1 FROM Tape_File T, Temp_Remove_CASTOR_Metadata F
     WHERE T.archive_file_id = F.archive_file_id AND T.copy_nb = 2);
  IF varIs2ndCopy = 1 THEN
    -- yes, work out the other tapepool to be removed
    SELECT archive_file_id INTO varTempId
      FROM Temp_Remove_CASTOR_Metadata
     WHERE ROWNUM = 1;
    SELECT tape_pool_name, tape_pool_id
      INTO var2ndTapePool, var2ndTapePoolId
      FROM Tape_Pool
     WHERE tape_pool_id = (
        SELECT tape_pool_id FROM Tape
         WHERE tape_pool_id <> varTapePoolId
           AND vid IN (SELECT vid FROM Tape_File
                        WHERE archive_file_id = varTempId)
        );
    CNS_ctaLog(inTapePool, 'Dual tape copies detected, will ALSO remove files metadata from tape pool '|| var2ndTapePool);
    CNS_ctaLog(var2ndTapePool, 'Removal of CASTOR files and tapes metadata due to the removal of tape pool '|| inTapePool);
  END IF;
  -- efficiently delete all Tape_File and Archive_File entries in multiple bulks
  OPEN temp_data_cur;
  LOOP
    FETCH temp_data_cur BULK COLLECT INTO ids LIMIT 10000;
    EXIT WHEN ids.count = 0;
    FORALL i IN 1..ids.count
      DELETE FROM Tape_File WHERE archive_file_id = ids(i);
    FORALL i IN 1..ids.count
      DELETE FROM Archive_File WHERE archive_file_id = ids(i);
  END LOOP;
  CLOSE temp_data_cur;
  -- delete all CASTOR tapes but leave the tapepool(s) in the system
  DELETE FROM Tape WHERE (tape_pool_id = varTapePoolId OR tape_pool_id = var2ndTapePoolId) AND is_from_castor = '1';
  -- commit the entire operation: this will clean the temporary table
  COMMIT;
  CNS_ctaLog(inTapePool, 'Removal of CASTOR tapes metadata completed successfully');
END;
/


-- The following is to be executed at schema creation or before the first migration
BEGIN
  dbms_errlog.create_error_log(dml_table_name => 'Tape_File');
EXCEPTION
  WHEN OTHERS THEN
    IF SQLCODE = -955 THEN
      NULL;  -- suppresses ORA-00955 name is already used by an existing object
    ELSE
      RAISE;
    END IF;
END;
/
