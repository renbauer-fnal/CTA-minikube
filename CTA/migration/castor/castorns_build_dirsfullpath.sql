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

DROP TABLE CTADirsFullPathNames;
DROP TABLE CTADirsFullPathMigration;

/* Subset of the CNS_File_Metadata table containing directories */
CREATE TABLE CTADirsFullPathNames AS SELECT * FROM cns_file_metadata WHERE BITAND(filemode, 16384) = 16384;

/* New table containing directories with full path, built in depth order */
CREATE TABLE CTADirsFullPathMigration AS SELECT * FROM CTADirsFullPathNames WHERE parent_fileid = 0;
ALTER TABLE CTADirsFullPathMigration ADD(depth integer, num_files integer);
ALTER TABLE CTADirsFullPathMigration MODIFY NAME varchar2(1024);
ALTER TABLE CTADirsFullPathMigration ADD CONSTRAINT PK_DIRNAMES PRIMARY KEY (fileid);

UPDATE CTADirsFullPathMigration set depth=0, num_files = 0;
CREATE INDEX dirnames_depth on CTADirsFullPathMigration(depth);
CREATE INDEX dirs_parent on CTADirsFullPathNames(parent_fileid);
INSERT INTO CTADirsFullPathMigration select a.fileid, a.parent_fileid, '/' || a.name, a.filemode, a.nlink,
  a.owner_uid, a.gid, a.filesize, a.atime, a.mtime, a.ctime, a.status, a.fileclass, a.guid, a.csumtype,
  a.csumvalue, a.acl, a.stagertime, a.oncta, b.depth+1, 0 from CTADirsFullPathNames a, CTADirsFullPathMigration b
  where a.parent_fileid=b.fileid and b.depth=0;

/* Table should contain 2 rows at depth 0 and 1 */
SELECT * from CTADirsFullPathMigration;

BEGIN
  FOR Curdepth IN 1..40
  LOOP
    INSERT INTO CTADirsFullPathMigration
        (SELECT a.fileid, a.parent_fileid, b.name || '/' || a.name, a.filemode, a.nlink, a.owner_uid,
                a.gid, a.filesize, a.atime, a.mtime, a.ctime, a.status, a.fileclass, a.guid, a.csumtype,
                a.csumvalue, a.acl, a.stagertime, a.oncta, b.depth+1, 0
         FROM CTADirsFullPathNames a, CTADirsFullPathMigration b
         WHERE a.parent_fileid=b.fileid and b.depth=Curdepth);
    EXIT WHEN sql%rowcount = 0;
  END LOOP;
END;

TRUNCATE TABLE Dirs_Full_Path;
INSERT INTO Dirs_Full_Path
      (SELECT fileid, parent_fileid as parent, SUBSTR(name,(INSTR(name,'/',-1,1)+1),length(name)) as name, name as path, depth
       FROM CTADirsFullPathMigration);
