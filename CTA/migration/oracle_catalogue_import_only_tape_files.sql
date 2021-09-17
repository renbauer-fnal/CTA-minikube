
-- Insert the file-level metadata for the given migration
CREATE OR REPLACE PROCEDURE populateCTAFilesFromCASTOR(inEOSCTAInstance VARCHAR2, inTapePool VARCHAR2, inVO_id INTEGER) AS
  CONSTRAINT_VIOLATED EXCEPTION;
  PRAGMA EXCEPTION_INIT(CONSTRAINT_VIOLATED, -1);
  varIs2ndCopy INTEGER;
  nbPreviousErrors INTEGER;
  nbMissingImports INTEGER;
BEGIN
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
       SET data_in_bytes = data_in_bytes + v.totalpervid
     WHERE vid = v.vid;
  END LOOP;
  COMMIT;
  CNS_ctaLog(inTapePool, 'Updated Tape data counters');
END;
/
