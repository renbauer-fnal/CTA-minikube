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

UNDEF ctaSchema
ACCEPT ctaSchema CHAR PROMPT 'Enter the username of the CTA schema: ';

-- Helper tables to store intermediate data for the export
CREATE TABLE CTAFilesHelper (fileid INTEGER NOT NULL PRIMARY KEY, parent_fileid INTEGER, filename VARCHAR2(255), disk_uid INTEGER, disk_gid INTEGER,
                             filemode INTEGER, btime INTEGER, ctime INTEGER, mtime INTEGER, classid INTEGER,
                             filesize INTEGER, checksum NUMBER, copyno INTEGER, vid VARCHAR2(6), fseq INTEGER,
                             blockId INTEGER, s_mtime INTEGER);
CREATE INDEX I_CTAFiles_parent_id ON CTAFilesHelper (parent_fileid);
CREATE INDEX I_CTAFiles_vid_filesize ON CTAFilesHelper (vid, filesize);
CREATE INDEX I_CTAFiles_copyno ON CTAFilesHelper (copyno);
CREATE INDEX I_CTAFiles_classid ON CTAFilesHelper (classid);

CREATE TABLE CTAFiles2ndCopyHelper (fileid INTEGER NOT NULL PRIMARY KEY, filesize INTEGER,
                                    vid VARCHAR2(6), fseq INTEGER, blockId INTEGER, s_mtime INTEGER);
CREATE INDEX I_CTAFiles2_vid_filesize ON CTAFiles2ndCopyHelper (vid, filesize);

CREATE TABLE CTAFilesFailed (fileid INTEGER NOT NULL PRIMARY KEY, parent_fileid INTEGER, filename VARCHAR2(255), disk_uid INTEGER, disk_gid INTEGER,
                             filemode INTEGER, btime INTEGER, ctime INTEGER, mtime INTEGER, classid INTEGER,
                             filesize INTEGER, checksum NUMBER, copyno INTEGER, VID VARCHAR2(6), fseq INTEGER,
                             blockId INTEGER, s_mtime INTEGER, retc INTEGER, message VARCHAR2(2048));
ALTER TABLE CTAFilesFailed ENABLE ROW MOVEMENT;
CREATE INDEX I_CTAFilesFailed_parent_id ON CTAFilesFailed (parent_fileid);
CREATE INDEX I_CTAFilesFailed_vid_filesize ON CTAFilesFailed (vid, filesize);

CREATE GLOBAL TEMPORARY TABLE CTAFilesFailedTemp(
  fileid INTEGER NOT NULL PRIMARY KEY, parent_fileid INTEGER, filename VARCHAR2(255), disk_uid INTEGER, disk_gid INTEGER, filemode INTEGER,
  btime INTEGER, ctime INTEGER, mtime INTEGER, classid INTEGER, filesize INTEGER, checksum NUMBER, copyno INTEGER, VID VARCHAR2(6), fseq INTEGER,
  blockId INTEGER, s_mtime INTEGER, retc INTEGER, message VARCHAR2(2048))
ON COMMIT DELETE ROWS;

CREATE OR REPLACE VIEW CTADirsHelper AS
  SELECT F.fileid, F.parent_fileid, D.depth, substr(D.path, length('/castor/cern.ch/')) as path,
         F.owner_uid disk_uid, F.gid disk_gid,
         F.filemode, F.ctime, F.mtime, F.fileclass classid       -- no creation time available for directories, cf. #556
    FROM Cns_file_metadata F, Dirs_Full_Path D
   WHERE F.fileid = D.fileid;

CREATE TABLE CTADeltaDirsHelper (fileid INTEGER NOT NULL PRIMARY KEY, parent_fileid INTEGER, depth INTEGER,
                                 path VARCHAR2(2048), disk_uid INTEGER, disk_gid INTEGER, filemode INTEGER,
                                 ctime INTEGER, mtime INTEGER, classid INTEGER);

CREATE TABLE CTADirsFailed(
  fileid INTEGER NOT NULL PRIMARY KEY, depth INTEGER, path VARCHAR2(2048), disk_uid INTEGER, disk_gid INTEGER,
  filemode INTEGER, ctime INTEGER, mtime INTEGER, classid INTEGER, retc INTEGER, message VARCHAR2(2048));
ALTER TABLE CTADirsFailed ENABLE ROW MOVEMENT;

CREATE GLOBAL TEMPORARY TABLE CTADirsFailedTemp(
  fileid INTEGER NOT NULL PRIMARY KEY, depth INTEGER, path VARCHAR2(2048), disk_uid INTEGER, disk_gid INTEGER,
  filemode INTEGER, ctime INTEGER, mtime INTEGER, classid INTEGER, retc INTEGER, message VARCHAR2(2048))
ON COMMIT DELETE ROWS;

-- Table to store the logs of the migration operations
CREATE TABLE CTAMigrationLog (timestamp NUMBER, seq INTEGER, client VARCHAR2(100), tapepool VARCHAR2(15), message VARCHAR2(2000));
CREATE SEQUENCE CTAMigrationLog_Seq START WITH 1 INCREMENT BY 1;

-- Internal procedure to log an action for a given migration process
CREATE OR REPLACE PROCEDURE ctaLog(inTapePool IN VARCHAR2, inMsg IN VARCHAR2) AS
PRAGMA AUTONOMOUS_TRANSACTION;
BEGIN
  INSERT INTO CTAMigrationLog (timestamp, seq, client, tapepool, message)
    VALUES (getTime(), CTAMigrationLog_Seq.nextval,
            (SELECT SYS_CONTEXT('USERENV', 'HOST', 100) FROM Dual),
            inTapePool, inMsg);
  COMMIT;
END;
/

-- Helper function to pretty print an ETA
CREATE OR REPLACE FUNCTION prettyTime(inMins IN INTEGER) RETURN VARCHAR2 DETERMINISTIC AS
BEGIN
  IF inMins < 60 THEN
    RETURN round(inMins) || ' minutes';
  ELSE
    RETURN trunc(inMins/60) || ' hours ' || trunc(mod(inMins, 60)) || ' mins';
  END IF;
END;
/


/* Procedure to prepare the files and segments metadata for export to CTA */
CREATE OR REPLACE PROCEDURE filesForCTAExport(inPoolName IN VARCHAR2, out2ndCopyPoolName OUT VARCHAR2) AS
  nbFiles INTEGER;
  var2ndCopy INTEGER;
BEGIN
  -- populate the helper table by selecting all required metadata
  INSERT INTO CTAFilesHelper (
    SELECT F.fileid, F.parent_fileid, F.name filename,    -- the full path is later built in CTA
           F.owner_uid disk_uid, F.gid disk_gid,
           F.filemode, F.atime, F.ctime, F.mtime, F.fileclass classid,   -- atime in CASTOR is creation time (#556)
           S.segsize as filesize, S.checksum, S.copyno, S.vid,
           S.fseq, utl_raw.cast_to_binary_integer(S.blockId), S.lastModificationTime as s_mtime
      FROM Cns_file_metadata F, Cns_seg_metadata S
     WHERE F.fileid = S.s_fileid
       AND S.vid IN (
        SELECT vid FROM Vmgr_tape_side
         WHERE poolName = inPoolName
           AND BITAND(status, 2) = 0 AND BITAND(status, 32) = 0    -- not already EXPORTED or ARCHIVED
       )
    );
  COMMIT;
  SELECT COUNT(*) INTO nbFiles FROM CTAFilesHelper;
  IF nbFiles = 0 THEN
    raise_application_error(-20000, 'No such tape pool or no valid files found, aborting the export');
  END IF;
  ctaLog(inPoolName, 'Intermediate table for files prepared, '|| nbFiles ||' files to be exported. ETA: '||
                     prettyTime(nbFiles/5000/60));

  -- check for dual tape copies if any: first, abort if this tape pool already contains some 2nd copies
  SELECT COUNT(*) INTO nbFiles FROM CTAFilesHelper WHERE copyno <> 1 AND copyno IS NOT NULL;
  IF nbFiles > 0 THEN
    raise_application_error(-20000, 'Second tape copy detected, please export the first-copy tape pool to also export this one');
  END IF;
  -- now look for the second copy tape pool if the involved fileclasses require it to exist
  out2ndCopyPoolName := NULL;
  SELECT MAX(nbcopies) INTO var2ndCopy FROM Cns_class_metadata
   WHERE classid IN (SELECT DISTINCT classid FROM CTAFilesHelper);
  IF var2ndCopy = 2 THEN
    -- yes, work out the tape pool holding the second copies
    BEGIN
      SELECT DISTINCT(poolName) INTO out2ndCopyPoolName
        FROM Vmgr_tape_side T
      WHERE T.vid IN (
        SELECT S.vid
          FROM Cns_seg_metadata S, CTAFilesHelper F
         WHERE S.s_fileid = F.fileid
           AND S.copyno = 2
      );
    EXCEPTION WHEN TOO_MANY_ROWS THEN
      -- more than one tape pool found for the second copies of the selected files
      raise_application_error(-20000, 'Multiple tape pools found, which hold the second copy for files in '||
                              inPoolName ||'. This is not supported in CTA, aborting.');
    END;
    ctaLog(inPoolName, 'Dual tape copies detected, will also export tape pool '|| out2ndCopyPoolName);
    ctaLog(out2ndCopyPoolName, 'Preparing export of the second-copy tape pool following export of '|| inPoolName);
    -- populate the 2nd copy helper table
    INSERT INTO CTAFiles2ndCopyHelper (
      SELECT s_fileid, segsize, vid, fseq, utl_raw.cast_to_binary_integer(blockId), lastModificationTime
        FROM Cns_seg_metadata S
       WHERE vid IN (
         SELECT vid FROM Vmgr_tape_side
         WHERE poolName = out2ndCopyPoolName
           AND BITAND(status, 2) = 0 AND BITAND(status, 32) = 0    -- not already EXPORTED or ARCHIVED
         )
      );
    COMMIT;
    ctaLog(inPoolName, 'Intermediate table for second-copy files prepared');
  ELSE
    ctaLog(inPoolName, 'Search for dual tape copies completed');
  END IF;
END;
/

/* Procedure to prepare the empty files metadata for export to CTA */
CREATE OR REPLACE PROCEDURE zeroByteFilesForCTAExport(inVO IN VARCHAR2) AS
  nbFiles INTEGER;
BEGIN
  -- look for 0-byte files, matching their top-level path(s) to the given VO.
  -- This assumption holds true for the medium/large VOs (LHC and AMS, COMPASS, etc.),
  -- but not for all the rest (i.e. general-purpose data, backups, public user areas, etc.).
  -- For those, a separate migration needs to be done at the very end of the process by taking
  -- any non-migrated files left over (passing inVO = 'ALL').
  INSERT INTO CTAFilesHelper
    (fileid, parent_fileid, filename, disk_uid, disk_gid, filemode,
    btime, ctime, mtime, classid, filesize, checksum) (
    SELECT F.fileid, F.parent_fileid, F.name, F.owner_uid, F.gid,
          F.filemode, F.atime, F.ctime, F.mtime, F.fileclass, 0, 1
      FROM Cns_file_metadata F, Dirs_Full_Path D
    WHERE F.parent_fileid = D.fileid
      AND F.filesize = 0 AND F.onCTA IS NULL   -- zero-byte file not yet on CTA
      AND BITAND(F.filemode, 16384) = 0        -- not a directory
      AND F.fileclass IS NOT NULL              -- nor a symbolic link
      AND (D.path LIKE '/castor/cern.ch/' || lower(inVO) || '%'
        OR D.path LIKE '/castor/cern.ch/grid/' || lower(inVO) || '%'   -- belongs to the given VO
        OR inVO = 'ALL')                  -- catch-all keyword, cf. README.md
    );
  COMMIT;
  SELECT COUNT(*) INTO nbFiles FROM CTAFilesHelper;
  IF nbFiles = 0 THEN
    raise_application_error(-20000, 'No valid files found, aborting the export');
  END IF;
  ctaLog(upper(inVO) || '::*', 'Intermediate table for empty files prepared, '|| nbFiles ||' files to be exported. ETA: '||
                        prettyTime(nbFiles/6000/60));
END;
/


/* Procedure to prepare the directories for export to CTA. The local Dirs_Full_Path cache table is updated */
CREATE OR REPLACE PROCEDURE dirsForCTAExport(inPoolName IN VARCHAR2) AS
  varDirsCount INTEGER;
  dirsSnapshotTime INTEGER;
  dirIds numList;
  varPath VARCHAR2(2048);
BEGIN
  EXECUTE IMMEDIATE 'TRUNCATE TABLE CTADeltaDirsHelper';
  -- estimate the last update time of the Dirs_Full_Path snapshot. To be noted that
  -- in case the directory identified by max(fileid) got updated itself, the estimate
  -- will be incorrect: we just hope this is unlikely to happen as renames are very rare.
  SELECT mtime INTO dirsSnapshotTime FROM Cns_file_metadata
   WHERE fileid = (SELECT max(fileid) FROM Dirs_Full_Path);
  -- select any parent directory of the selected files, whose mtime or ctime has changed after
  -- Dirs_Full_Path was last updated: this typically happens because of newly added files
  -- but also because of renames!
  SELECT DISTINCT(D.fileid)
    BULK COLLECT INTO dirIds
    FROM CTAFilesHelper F, Cns_file_metadata D
   WHERE F.parent_fileid = D.fileid
     AND (D.mtime > dirsSnapshotTime OR D.ctime > dirsSnapshotTime);
  IF dirIds.count > 0 THEN
    -- found some, delete the renamed paths from the hierarchy snapshot
    ctaLog(inPoolName, 'Identified '|| dirIds.count ||' newly created/renamed directories');
    FOR i IN 1..dirIds.count LOOP
      BEGIN
        SELECT path INTO varPath FROM Dirs_Full_Path WHERE fileid = dirIds(i);
        DELETE FROM Dirs_Full_Path
         WHERE path LIKE varPath || '%';
        COMMIT;
      EXCEPTION WHEN NO_DATA_FOUND THEN
        -- this was a newly created path, will be inserted later
        NULL;
      END;
    END LOOP;
  END IF;
  -- do the selection again, this time with the anti-join to exactly identify
  -- which directories are missing for this export
  SELECT DISTINCT(D.fileid)
    BULK COLLECT INTO dirIds
    FROM CTAFilesHelper F, Cns_file_metadata D
   WHERE F.parent_fileid = D.fileid
     AND NOT EXISTS (SELECT 1 FROM Dirs_Full_Path DP WHERE DP.fileid = D.fileid);
  IF dirIds.count = 0 THEN
    ctaLog(inPoolName, 'Search for missing directories completed');
    RETURN;
  END IF;
  -- now populate the delta directories helper table with any directory that
  -- is not present in the Dirs_Full_Path snapshot. This accounts both for renames
  -- and for newly created directories after the Dirs_Full_Path was updated. Deleted
  -- directories are ignored
  ctaLog(inPoolName, 'Identified '|| dirIds.count ||' missing directories, preparing intermediate delta table');
  FOR i IN 1..dirIds.count LOOP
    MERGE INTO CTADeltaDirsHelper DD
      USING (SELECT /*+ NO_CONNECT_BY_COST_BASED */
                    fileid, parent_fileid, getPathDepthForFileId(fileid) as depth,
                    substr(getPathForFileId(fileid), length('/castor/cern.ch/')) as path,
                    owner_uid disk_uid, gid disk_gid,
                    filemode, ctime, mtime, fileclass classid
               FROM Cns_file_metadata
               START WITH fileid = dirIds(i)
               CONNECT BY fileid = PRIOR parent_fileid) D
    ON (DD.fileid = D.fileid)
    WHEN NOT MATCHED THEN
      INSERT (fileid, parent_fileid, depth, path, disk_uid, disk_gid,
              filemode, ctime, mtime, classid)
      VALUES (D.fileid, D.parent_fileid, D.depth, D.path, D.disk_uid, D.disk_gid,
              D.filemode, D.ctime, D.mtime, D.classid);
    COMMIT;
  END LOOP;
  -- drop null paths due to the above recursive query going to the top /castor level
  DELETE FROM CTADeltaDirsHelper WHERE path IS NULL;
  COMMIT;
  SELECT count(*) INTO varDirsCount FROM CTADeltaDirsHelper;
  ctaLog(inPoolName, 'Intermediate delta table completed with '|| varDirsCount ||' directories');
  -- also update the hierarchy snapshot with what was found
  MERGE INTO Dirs_Full_Path DP
    USING (SELECT fileid, parent_fileid, path, depth
             FROM CTADeltaDirsHelper) D
    ON (DP.fileid = D.fileid)
  WHEN MATCHED THEN
    UPDATE SET DP.parent = D.parent_fileid,
               DP.path = '/castor/cern.ch' || D.path,
               DP.depth = D.depth
  WHEN NOT MATCHED THEN
    -- note that here we leave D.name = NULL: it's not used by the CTA migration,
    -- and this way the extra entries are 'earmarked' for future analysis
    INSERT (fileid, parent, path, depth)
    VALUES (D.fileid, D.parent_fileid, '/castor/cern.ch' || D.path, D.depth);
  COMMIT;
  ctaLog(inPoolName, 'Updated directories snapshot');
END;
/


/* Procedure to terminate the export to CTA and account it on the statistics */
CREATE OR REPLACE PROCEDURE completeCTAExport AS
  CURSOR c IS SELECT fileid FROM CTAFilesHelper;
  ids numList;
  varUnused INTEGER;
  varTapePool VARCHAR2(100);
  var2ndTapePool VARCHAR2(100);
  varStartMsg VARCHAR2(2048);
  varStartTime INTEGER;
BEGIN
  BEGIN
    SELECT fileid INTO varUnused FROM CTAFilesHelper WHERE ROWNUM <= 1;
  EXCEPTION WHEN NO_DATA_FOUND THEN
    raise_application_error(-20000, 'No ongoing files export to CTA, nothing to do');
  END;
  -- Get info from the current export. It must be there as we passed the previous check.
  SELECT tapePool, timestamp, message INTO varTapePool, varStartTime, varStartMsg
    FROM (SELECT tapePool, timestamp, message
            FROM CTAMigrationLog
           WHERE message LIKE 'CASTOR metadata import started%'
          ORDER BY timestamp DESC)
    WHERE ROWNUM <= 1;
  IF INSTR(varStartMsg, '[dry-run mode]') = 0 THEN
    -- This was a real export, therefore flag files and tapes in CASTOR accordingly
    ctaLog(varTapePool, 'Marking CASTOR files as exported to CTA');
    OPEN c;
    LOOP
      FETCH c BULK COLLECT INTO ids LIMIT 10000;
      EXIT WHEN ids.count = 0;
      FORALL i IN 1..ids.count
        UPDATE Cns_file_metadata F
           SET onCTA = 1 WHERE fileid = ids(i);
      FORALL i IN 1..ids.count
        UPDATE Cns_seg_metadata S
           SET onCTA = 1 WHERE s_fileid = ids(i);
      COMMIT;
    END LOOP;
    CLOSE c;
    -- now mark tapes as EXPORTED in VMGR
    ctaLog(varTapePool, 'Marking VMGR tapes as EXPORTED');
    UPDATE Vmgr_tape_side
       SET status = status + 2 - BITAND(status, 2)   -- i.e. status = BITOR(status, 2), but BITOR does not exist
     WHERE poolName = varTapePool;
    BEGIN
      -- look for the 2nd copy tape pool if any
      SELECT poolName INTO var2ndTapePool
        FROM Vmgr_tape_side
       WHERE vid IN (SELECT vid FROM CTAFiles2ndCopyHelper WHERE ROWNUM = 1);
      ctaLog(var2ndTapePool, 'Marking VMGR tapes as EXPORTED');
      UPDATE Vmgr_tape_side
         SET status = status + 2 - BITAND(status, 2)   -- i.e. status = BITOR(status, 2), but BITOR does not exist
       WHERE poolName = var2ndTapePool;
    EXCEPTION WHEN NO_DATA_FOUND THEN
      NULL;    -- no second copy tape pool to process
    END;
    COMMIT;
  END IF;
  EXECUTE IMMEDIATE 'TRUNCATE TABLE CTAFilesHelper';
  EXECUTE IMMEDIATE 'TRUNCATE TABLE CTAFiles2ndCopyHelper';
  IF INSTR(varStartMsg, '[dry-run mode]') > 0 THEN
    ctaLog(varTapePool, 'Export from CASTOR fully completed in '|| prettyTime((getTime()-varStartTime)/60) ||' [dry-run mode]');
  ELSE
    ctaLog(varTapePool, 'Export from CASTOR fully completed in '|| prettyTime((getTime()-varStartTime)/60));
  END IF;
END;
/


-- Need to grant access from the CTA catalogue schema
GRANT SELECT ON CTAFilesHelper TO &ctaSchema;
GRANT SELECT ON CTAFiles2ndCopyHelper TO &ctaSchema;
GRANT SELECT ON CTADirsHelper TO &ctaSchema;
GRANT SELECT ON CTAMigrationLog TO &ctaSchema;
GRANT SELECT ON Cns_class_metadata TO &ctaSchema;
GRANT SELECT ON Dirs_Full_Path TO &ctaSchema;
GRANT EXECUTE ON filesForCTAExport TO &ctaSchema;
GRANT EXECUTE ON zeroBytefilesForCTAExport TO &ctaSchema;
GRANT EXECUTE ON dirsForCTAExport TO &ctaSchema;
GRANT EXECUTE ON ctaLog TO &ctaSchema;
GRANT EXECUTE ON getTime TO &ctaSchema;
