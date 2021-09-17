/*!
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2003-2021 CERN
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

CREATE TABLE Cns_class_metadata (classid NUMBER(5), name VARCHAR2(15), owner_uid NUMBER, gid NUMBER(6), min_filesize NUMBER, max_filesize NUMBER, flags NUMBER(2), maxdrives NUMBER(3), max_segsize NUMBER, migr_time_interval NUMBER, mintime_beforemigr NUMBER, nbcopies NUMBER(1), retenp_on_disk NUMBER);

CREATE TABLE Cns_tp_pool (classid NUMBER(5), tape_pool VARCHAR2(15));

CREATE TABLE Cns_file_metadata (fileid NUMBER, parent_fileid NUMBER, guid CHAR(36), name VARCHAR2(255), filemode NUMBER(6), nlink NUMBER, owner_uid NUMBER, gid NUMBER(6), filesize NUMBER, atime NUMBER(10), mtime NUMBER(10), ctime NUMBER(10), stagertime NUMBER NOT NULL, fileclass NUMBER(5), status CHAR(1), csumtype VARCHAR2(2), csumvalue VARCHAR2(32), acl VARCHAR2(3900), onCta INTEGER) STORAGE (INITIAL 5M NEXT 5M PCTINCREASE 0);

CREATE TABLE Cns_user_metadata (u_fileid NUMBER, comments VARCHAR2(255));

CREATE TABLE Cns_seg_metadata (s_fileid NUMBER, copyno NUMBER(1),fsec NUMBER(3), segsize NUMBER, compression NUMBER, s_status CHAR(1), vid VARCHAR2(6), side NUMBER (1), fseq NUMBER(10), blockid RAW(4), checksum_name VARCHAR2(16), checksum NUMBER, gid NUMBER(6), creationTime NUMBER, lastModificationTime NUMBER, onCta INTEGER);

CREATE TABLE Cns_symlinks (fileid NUMBER, linkname VARCHAR2(1023));

CREATE SEQUENCE Cns_unique_id START WITH 3 INCREMENT BY 1 CACHE 1000;

ALTER TABLE Cns_class_metadata
  ADD CONSTRAINT pk_classid PRIMARY KEY (classid)
  ADD CONSTRAINT classname UNIQUE (name);

ALTER TABLE Cns_file_metadata
  ADD CONSTRAINT pk_fileid PRIMARY KEY (fileid)
  ADD CONSTRAINT file_full_id UNIQUE (parent_fileid, name)
  ADD CONSTRAINT file_guid UNIQUE (guid)
    USING INDEX STORAGE (INITIAL 2M NEXT 2M PCTINCREASE 0);

ALTER TABLE Cns_user_metadata
  ADD CONSTRAINT pk_u_fileid PRIMARY KEY (u_fileid);

ALTER TABLE Cns_seg_metadata
  ADD CONSTRAINT pk_s_fileid PRIMARY KEY (s_fileid, copyno, fsec);

ALTER TABLE Cns_symlinks
  ADD CONSTRAINT pk_l_fileid PRIMARY KEY (fileid);

ALTER TABLE Cns_user_metadata
  ADD CONSTRAINT fk_u_fileid FOREIGN KEY (u_fileid)
  REFERENCES Cns_file_metadata (fileid);

ALTER TABLE Cns_file_metadata
  ADD CONSTRAINT fk_class FOREIGN KEY (fileclass)
  REFERENCES Cns_class_metadata (classid);

ALTER TABLE Cns_seg_metadata
  ADD CONSTRAINT fk_s_fileid FOREIGN KEY (s_fileid)
  REFERENCES Cns_file_metadata (fileid)
  ADD CONSTRAINT tapeseg UNIQUE (vid, side, fseq);

ALTER TABLE Cns_tp_pool
  ADD CONSTRAINT classpool UNIQUE (classid, tape_pool);

ALTER TABLE Cns_symlinks
  ADD CONSTRAINT fk_l_fileid FOREIGN KEY (fileid)
  REFERENCES Cns_file_metadata (fileid);

-- Constraint to prevent negative nlinks
ALTER TABLE Cns_file_metadata
  ADD CONSTRAINT CK_FileMetadata_Nlink
  CHECK (nlink >= 0 AND nlink IS NOT NULL);

-- Constraint to prevent a NULL file class on non symlink entries
ALTER TABLE Cns_file_metadata
  ADD CONSTRAINT CK_FileMetadata_FileClass
  CHECK ((bitand(filemode, 40960) = 40960  AND fileclass IS NULL)
     OR  (bitand(filemode, 40960) != 40960 AND fileclass IS NOT NULL));

-- Create indexes on Cns_file_metadata
CREATE INDEX Parent_FileId_Idx ON Cns_file_metadata (parent_fileid);
CREATE INDEX I_file_metadata_fileclass ON Cns_file_metadata (fileclass);
CREATE INDEX I_file_metadata_gid ON Cns_file_metadata (gid);

-- Create indexes on Cns_seg_metadata
CREATE INDEX I_seg_metadata_tapesum ON Cns_seg_metadata (vid, s_fileid, segsize, compression);
CREATE INDEX I_seg_metadata_gid ON Cns_Seg_metadata (gid);

-- Temporary table to support Cns_bulkexist calls
CREATE GLOBAL TEMPORARY TABLE Cns_files_exist_tmp
  (tmpFileId NUMBER) ON COMMIT DELETE ROWS;

-- Table to store configuration items at the DB level
CREATE TABLE CastorConfig
  (class VARCHAR2(50) CONSTRAINT NN_CastorConfig_class NOT NULL,
   key VARCHAR2(50) CONSTRAINT NN_CastorConfig_key NOT NULL,
   value VARCHAR2(100) CONSTRAINT NN_CastorConfig_value NOT NULL,
   description VARCHAR2(1000));
ALTER TABLE CastorConfig ADD CONSTRAINT UN_CastorConfig_class_key UNIQUE (class, key);

-- Tables to store intermediate data to be passed from/to the stager.
-- Note that we cannot use temporary tables with distributed transactions.
CREATE TABLE SetSegsForFilesInputHelper
  (reqId VARCHAR2(36), fileId NUMBER, lastModTime NUMBER, copyNo NUMBER, oldCopyNo NUMBER, transfSize NUMBER,
   comprSize NUMBER, vid VARCHAR2(6), fseq NUMBER, blockId RAW(4), checksumType VARCHAR2(16), checksum NUMBER);

CREATE TABLE SetSegsForFilesResultsHelper
  (isOnlyLog NUMBER(1), reqId VARCHAR2(36), timeInfo NUMBER, errorCode INTEGER, fileId NUMBER, msg VARCHAR2(2048), params VARCHAR2(4000));

-- Table to cache the full path of all directories, used to speed up the CTA export.
-- This is manually populated in production with the nsDirsFullPath.sql script.
CREATE TABLE Dirs_Full_Path (fileid NUMBER NOT NULL,
                             parent NUMBER NOT NULL,
                             name VARCHAR2 (255),
                             path VARCHAR2 (2048),
                             depth INTEGER);
CREATE INDEX I_Dirs_Full_Path_Fileid on Dirs_Full_Path (fileid);

/* Get current time as a time_t. Not that easy in ORACLE */
CREATE OR REPLACE FUNCTION getTime RETURN NUMBER IS
  epoch            TIMESTAMP WITH TIME ZONE;
  now              TIMESTAMP WITH TIME ZONE;
  interval         INTERVAL DAY(9) TO SECOND;
  interval_days    NUMBER;
  interval_hours   NUMBER;
  interval_minutes NUMBER;
  interval_seconds NUMBER;
BEGIN
  epoch := TO_TIMESTAMP_TZ('01-JAN-1970 00:00:00 00:00',
    'DD-MON-YYYY HH24:MI:SS TZH:TZM');
  now := SYSTIMESTAMP AT TIME ZONE '00:00';
  interval         := now - epoch;
  interval_days    := EXTRACT(DAY    FROM (interval));
  interval_hours   := EXTRACT(HOUR   FROM (interval));
  interval_minutes := EXTRACT(MINUTE FROM (interval));
  interval_seconds := EXTRACT(SECOND FROM (interval));

  RETURN interval_days * 24 * 60 * 60 + interval_hours * 60 * 60 +
    interval_minutes * 60 + interval_seconds;
END;
/

/* Insert the bare minimum to get a working system.
 * - Create a default 'system' fileclass. Pre-requisite to next step.
 * - Create the root directory.
 */
INSERT INTO Cns_class_metadata (classid, name, owner_uid, gid, min_filesize, max_filesize, flags, maxdrives,
  max_segsize, migr_time_interval, mintime_beforemigr, nbcopies, retenp_on_disk)
  VALUES (1, 'system', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
INSERT INTO cns_file_metadata (fileid, parent_fileid, guid, name, filemode, nlink, owner_uid, gid, filesize,
  atime, mtime, ctime, stagertime, fileclass, status, csumtype, csumvalue, acl)
  VALUES (2, 0, NULL,  '/', 16877, 0, 0, 0, 0, 0,  0, 0, 0, 1, '-', NULL, NULL, NULL);
COMMIT;

CREATE OR REPLACE PROCEDURE cns_ctaLog(inTapepool IN VARCHAR2, inMsg IN VARCHAR2) AS
BEGIN
  NULL;
END;
/
