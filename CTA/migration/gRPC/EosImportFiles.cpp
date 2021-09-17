/*
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2015-2021 CERN
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

#include <sys/stat.h>
#include <XrdSsiPbConfig.hpp>
#include <XrdSsiPbLog.hpp>
#include <rdbms/AutoRollback.hpp>
#include <rdbms/Login.hpp>
#include <common/checksum/ChecksumBlob.hpp>
#include "EosImportFiles.hpp"
#include "GrpcUtils.hpp"


namespace cta {
namespace migration {

std::exception_ptr EosImportFiles::m_exptr = nullptr;


EosImportFiles::EosImportFiles(const std::string &configfile, std::string storageClass, bool retry, bool check, uint64_t skip) :
  m_start_time(std::chrono::steady_clock::now()),
  m_total_files(0),
  m_is_retry(retry),
  m_is_check(check),
  m_skip(skip),
  m_storageClassFilter(storageClass),
  m_files_done(false)
{
  // Parse configuration file
  XrdSsiPb::Config config(configfile);

  auto is_json       = config.getOptionValueBool("castor.json");
  auto dbconn        = config.getOptionValueStr("castor.db_login");
  auto max_num_conns = config.getOptionValueInt("castor.max_num_connections");
  auto batch_size    = config.getOptionValueInt("castor.batch_size");
  auto castor_prefix = config.getOptionValueStr("castor.prefix");
  auto is_dry_run    = config.getOptionValueBool("eos.dry_run");
  auto is_strict     = config.getOptionValueBool("eos.retry.strict_compare");
  auto eos_prefix    = config.getOptionValueStr("eos.prefix");
  auto endpoint      = config.getOptionValueStr("eos.endpoint");
  auto token         = config.getOptionValueStr("eos.token");
  auto eos_layout    = config.getOptionValueInt("eos.layout");

  // Determine which DB table to use
  if(retry) {
    m_tableName = "CTAFILESFAILED";
  } else {
    m_tableName = "CTAFILESHELPER";
  }

  // Connect to Oracle
  if(!dbconn.first) {
    throw std::runtime_error("castor.db_login must be specified in the config file in the form oracle:user/password@TNS");
  }
  const auto dbLogin = rdbms::Login::parseString(dbconn.second);
  const uint64_t min_num_conns = 2;  // One connection for file select and one for storage classes
  uint64_t actual_max_num_conns = min_num_conns;
  if(max_num_conns.first && ((uint64_t)max_num_conns.second) > min_num_conns) {
    actual_max_num_conns = max_num_conns.second;
  }
  m_dbConnPool = ::cta::make_unique<rdbms::ConnPool>(dbLogin, actual_max_num_conns);
  m_dbConn = m_dbConnPool->getConn();

  // Connect to EOS
  m_eosgrpc = eos::client::GrpcClient::Create(endpoint.first ? endpoint.second : "localhost:50051", token.second);

  // Default layout: see EOS common/LayoutId.hh for definitions of constants
  const int kAdler         =  0x2;
  const int kReplica       = (0x1 <<  4);
  const int kStripeSize    = (0x0 <<  8); // 1 stripe
  const int kStripeWidth   = (0x0 << 16); // 4K blocks
  const int kBlockChecksum = (0x1 << 20);
  
  // Default single replica layout id should be 00100012
  const uint64_t default_layout = kReplica | kAdler | kStripeSize | kStripeWidth | kBlockChecksum;


  // Set parameters and defaults
  m_is_json         = is_json.first       ? is_json.second       : false;
  m_is_dry_run      = is_dry_run.first    ? is_dry_run.second    : false;
  m_is_retry_strict = is_strict.first     ? is_strict.second     : true;
  m_batch_size      = batch_size.first    ? batch_size.second    : 10000;
  m_castor_prefix   = castor_prefix.first ? castor_prefix.second : "/castor/cern.ch/";
  m_eos_prefix      = eos_prefix.first    ? eos_prefix.second    : "/eos/grpc/";
  m_layout          = eos_layout.first    ? eos_layout.second    : default_layout;
  // enforce a slash at beginning and end of prefixes
  eos::client::checkPrefix(m_castor_prefix);
  eos::client::checkPrefix(m_eos_prefix);
}


void EosImportFiles::listFailureModes()
{
  const std::string sql = "SELECT"
    "  RETC,"
    "  REGEXP_REPLACE(REGEXP_REPLACE(MESSAGE, 'path=.*:', 'path=<path>:'), 'id=[0-9]*', 'id=<id>') AS MSG, "
    "  COUNT(*) AS CNT"
    "  FROM CTAFILESFAILED"
    "  GROUP BY RETC,"
    "  REGEXP_REPLACE(REGEXP_REPLACE(MESSAGE, 'path=.*:', 'path=<path>:'), 'id=[0-9]*', 'id=<id>') "
    "  ORDER BY RETC, MSG";
  auto stmt = m_dbConn.createStmt(sql);
  auto rset = stmt.executeQuery();

  while(rset.next()) {
    std::cout << rset.columnString("MSG") << " (retc=" << rset.columnUint64("RETC")
              << ", count=" << rset.columnUint64("CNT") << ")" << std::endl;
  }
}


void EosImportFiles::listFailures()
{
  const std::string sql = "SELECT"
    "  A.FILEID, B.PATH, A.FILENAME, A.RETC, A.MESSAGE"
    "  FROM CTAFILESFAILED A, CTADIRSHELPER B"
    "  WHERE A.PARENT_FILEID = B.FILEID";
  m_selectFilesDbStmt = m_dbConn.createStmt(sql);
  auto stmt = m_dbConn.createStmt(sql);
  auto rset = stmt.executeQuery();

  while(rset.next()) {
    std::cout << rset.columnString("FILEID") << "  "
              << rset.columnString("PATH") << "/" << rset.columnString("FILENAME") << "  "
              << rset.columnString("MESSAGE") << " (retc=" << rset.columnUint64("RETC") << ")" << std::endl;
  }
}


void EosImportFiles::forgetFailures()
{
  const std::string sql = "DELETE FROM CTAFILESFAILED";
  auto stmt = m_dbConn.createStmt(sql);
  std::cerr << "Deleting from CTAFILESFAILED...";
  stmt.executeNonQuery();
  std::cerr << "done." << std::endl;
}


void EosImportFiles::preFlightCheck()
{
  std::cerr << "Pre-flight check...";

  // Check EOS namespace file ID is set to a suitably high value
  std::cerr << "EOS namespace...";
  uint64_t eos_cid;
  uint64_t eos_fid;

  m_eosgrpc->GetCurrentIds(eos_cid, eos_fid);
  std::cerr << "(" << eos_cid << "," << eos_fid << ")...";
  if(eos_fid < EOS_FID_WARN) {
    std::cerr << std::endl << TEXT_RED
              << "WARNING: EOS current file ID < " << EOS_FID_WARN << TEXT_NORMAL << std::endl;
  }

  // Check that CTAFILESFAILED is empty
  if(!m_is_retry) {
    std::cerr << "DB...";
    const std::string sql_select = "SELECT COUNT(*) AS CNT FROM CTAFILESFAILED";
    auto stmt = m_dbConn.createStmt(sql_select);
    auto rset = stmt.executeQuery();
    rset.next();
    if(rset.columnUint64("CNT") != 0) {
      std::cerr << std::endl << TEXT_RED
                << "WARNING: CTAFILESFAILED is not empty. You should either:" << std::endl
                << "  1. Fix the problem(s) and retry : see --failure-modes, --list-failures and --retry options" << std::endl
                << "  2. Clear the saved failures : see --forget-failures option" << TEXT_NORMAL << std::endl;
    }
  }

  std::cerr << "done." << std::endl;

  // Report NS check and skipped files
  if(m_is_check) {
    std::cerr << "EOS namespace metadata will be checked before injecting each file." << std::endl;
  }
  if(m_skip > 0) {
    std::cerr << "Skipping first " << m_skip << " rows." << std::endl;
  }
}


void EosImportFiles::getStorageClasses()
{
  std::cerr << "Reading storage classes from CASTOR DB...";

  const std::string sql = "SELECT CLASSID, NAME FROM CNS_CLASS_METADATA";
  auto conn = m_dbConnPool->getConn();
  auto stmt = conn.createStmt(sql);
  auto rset = stmt.executeQuery();

  while(rset.next()) {
    const auto sc = std::make_pair(rset.columnUint64("CLASSID"), rset.columnString("NAME"));
    m_storageClass.insert(sc);
  }

  // Store reverse lookup as well (for error handling)
  for(auto &sc : m_storageClass) {
    const auto sc_rev = std::make_pair(sc.second, std::to_string(sc.first));
    m_storageClassRev.insert(sc_rev);
  }

  std::cerr << "(" << m_storageClass.size() << ")...done." << std::endl;
}


void EosImportFiles::select()
{
  std::string sql =
    "SELECT "
    "  A.FILEID,"
    "  A.PARENT_FILEID,"
    "  B.PATH,"
    "  A.FILENAME,"
    "  A.DISK_UID,"
    "  A.DISK_GID,"
    "  A.FILEMODE,"
    "  A.BTIME,"
    "  A.CTIME,"
    "  A.MTIME,"
    "  A.CLASSID,"
    "  A.FILESIZE,"
    "  A.CHECKSUM,"
    "  A.COPYNO,"
    "  A.VID,"
    "  A.FSEQ,"
    "  A.BLOCKID,"
    "  A.S_MTIME "
    "FROM " + m_tableName + " A, CTADIRSHELPER B "
    "WHERE A.PARENT_FILEID = B.FILEID";

  if(m_storageClassFilter.empty()) {
    m_selectFilesDbStmt = m_dbConn.createStmt(sql);
  } else {
    // Filter by storage class if this option was provided
    std::string sc;
    try {
      // Try looking up the string value in the list of known storage classes
      sc = m_storageClassRev.at(m_storageClassFilter);
    } catch(...) {
      // Assume that the numeric ID was provided
      sc = m_storageClassFilter;
    }
    auto scid = strtoul(sc.c_str(), NULL, 0);
    if(scid == 0) {
      throw std::runtime_error("Could not resolve \"" + m_storageClassFilter + "\" to a valid storage class");
    }

    sql += " AND A.CLASSID = :CLASSID";
    m_selectFilesDbStmt = m_dbConn.createStmt(sql);
    m_selectFilesDbStmt.bindUint64(":CLASSID", scid);
  }

  std::cerr << "Executing SELECT query against " << m_tableName << " table...";
  m_selectFilesDbRset = m_selectFilesDbStmt.executeQuery();
  std::cerr << "done." << std::endl;
}


void EosImportFiles::saveFailedFiles(const std::vector<eos::rpc::FileMdProto> &files,
  const eos::rpc::InsertReply &replies)
{
  std::cerr << "Updating list of failed inserts...";

  auto file_it = files.begin();
  auto retc_it = replies.retc().begin();
  auto message_it = replies.message().begin();

  // Put the set of files into a temporary table.
  cta::rdbms::AutoRollback autoRollback(m_dbConn);
  m_dbConn.setAutocommitMode(rdbms::AutocommitMode::AUTOCOMMIT_OFF);

  // This could be optimised by doing a batch insert.
  for( ; retc_it != replies.retc().end(); ++file_it, ++retc_it, ++message_it) {
    auto retc = *retc_it;

    // We only care about successes if we were reading from the failed table (because we need to
    // delete successful retries in this case)
    if(!m_is_retry && retc == 0) continue;

    // If the failure mode was "Attempted to create file with id=<id>, which already exists",
    // verify that the metadata in the EOS namespace is what we expect. If so, ignore the error and
    // treat it as a successful injection.
    if(m_is_retry && retc == EINVAL && compareMD(*file_it)) retc = 0;

    const std::string sql_insert = "INSERT INTO CTAFILESFAILEDTEMP("
      "  FILEID,"
      "  PARENT_FILEID,"
      "  FILENAME,"
      "  DISK_UID,"
      "  DISK_GID,"
      "  FILEMODE,"
      "  BTIME,"
      "  CTIME,"
      "  MTIME,"
      "  CLASSID,"
      "  FILESIZE,"
      "  CHECKSUM,"
      "  RETC,"
      "  MESSAGE) "
      "VALUES("
      "  :FILEID,"
      "  :PARENT_FILEID,"
      "  :FILENAME,"
      "  :DISK_UID,"
      "  :DISK_GID,"
      "  :FILEMODE,"
      "  :BTIME,"
      "  :CTIME,"
      "  :MTIME,"
      "  :CLASSID,"
      "  :FILESIZE,"
      "  :CHECKSUM,"
      "  :RETC,"
      "  :MESSAGE)";

    auto stmt = m_dbConn.createStmt(sql_insert);
    stmt.bindUint64(":FILEID", file_it->id());
    stmt.bindUint64(":PARENT_FILEID", file_it->cont_id());
    stmt.bindString(":FILENAME", file_it->name());
    stmt.bindUint64(":DISK_UID", file_it->uid());
    stmt.bindUint64(":DISK_GID", file_it->gid());
    stmt.bindUint64(":FILEMODE", file_it->flags());
    stmt.bindString(":BTIME", file_it->xattrs().at("eos.btime"));
    stmt.bindUint64(":CTIME", file_it->ctime().sec());
    stmt.bindUint64(":MTIME", file_it->mtime().sec());
    stmt.bindString(":CLASSID", m_storageClassRev.at(file_it->xattrs().at("sys.archive.storage_class")));
    stmt.bindUint64(":FILESIZE", file_it->size());
    stmt.bindUint64(":CHECKSUM", convertChecksum(file_it->checksum().value()));
    stmt.bindUint64(":RETC", retc);
    stmt.bindString(":MESSAGE", message_it->empty() ? "-" : *message_it);

    stmt.executeNonQuery();
  }

  // Merge the temporary table into the failure table
  if(m_is_retry) {
    // In the case of a retry, just handle deletions and updates
    std::cerr << "deleting...";
    const std::string sql_delete = "DELETE FROM CTAFILESFAILED "
      "WHERE FILEID IN (SELECT FILEID FROM CTAFILESFAILEDTEMP WHERE RETC = 0)";
    auto stmt_delete = m_dbConn.createStmt(sql_delete);
    stmt_delete.executeNonQuery();

    std::cerr << "updating...";
    const std::string sql_update = "UPDATE CTAFILESFAILED A SET("
      "  A.PARENT_FILEID,"
      "  A.FILENAME,"
      "  A.DISK_UID,"
      "  A.DISK_GID,"
      "  A.FILEMODE,"
      "  A.BTIME,"
      "  A.CTIME,"
      "  A.MTIME,"
      "  A.CLASSID,"
      "  A.FILESIZE,"
      "  A.CHECKSUM,"
      "  A.RETC,"
      "  A.MESSAGE) = ("
      "SELECT"
      "  B.PARENT_FILEID,"
      "  B.FILENAME,"
      "  B.DISK_UID,"
      "  B.DISK_GID,"
      "  B.FILEMODE,"
      "  B.BTIME,"
      "  B.CTIME,"
      "  B.MTIME,"
      "  B.CLASSID,"
      "  B.FILESIZE,"
      "  B.CHECKSUM,"
      "  B.RETC,"
      "  B.MESSAGE "
      "FROM CTAFILESFAILEDTEMP B "
      "WHERE A.FILEID = B.FILEID) "
      "WHERE EXISTS ("
      "  SELECT 1"
      "  FROM CTAFILESFAILEDTEMP B "
      "  WHERE A.FILEID = B.FILEID)";
    m_dbConn.setAutocommitMode(rdbms::AutocommitMode::AUTOCOMMIT_ON);
    auto stmt_update = m_dbConn.createStmt(sql_update);
    stmt_update.executeNonQuery();
  } else {
    // If it's not a retry, just handle insertions
    std::cerr << "inserting...";
    const std::string sql_insert = "INSERT INTO CTAFILESFAILED ("
      "  FILEID,"
      "  PARENT_FILEID,"
      "  FILENAME,"
      "  DISK_UID,"
      "  DISK_GID,"
      "  FILEMODE,"
      "  BTIME,"
      "  CTIME,"
      "  MTIME,"
      "  CLASSID,"
      "  FILESIZE,"
      "  CHECKSUM,"
      "  RETC,"
      "  MESSAGE) "
      "SELECT "
      "  FILEID,"
      "  PARENT_FILEID,"
      "  FILENAME,"
      "  DISK_UID,"
      "  DISK_GID,"
      "  FILEMODE,"
      "  BTIME,"
      "  CTIME,"
      "  MTIME,"
      "  CLASSID,"
      "  FILESIZE,"
      "  CHECKSUM,"
      "  RETC,"
      "  MESSAGE "
      "FROM CTAFILESFAILEDTEMP "
      "WHERE RETC != 0";
    m_dbConn.setAutocommitMode(rdbms::AutocommitMode::AUTOCOMMIT_ON);
    auto stmt_insert = m_dbConn.createStmt(sql_insert);
    stmt_insert.executeNonQuery();
  }
  std::cerr << "done." << std::endl;
}


bool EosImportFiles::compareMD(const eos::rpc::FileMdProto &file)
{
  auto remote_file = m_eosgrpc->GetMD(eos::rpc::FILE, file.id(), file.path());

  // If we send the string "adler" (which is what EOS uses internally), we expect to get back "adler32"
  // (because that's what XRootD expects and so EOS maps "adler"->"adler32" when it replies).
  if(remote_file.fmd().checksum().type() == "adler32") {
    remote_file.mutable_fmd()->mutable_checksum()->set_type("adler");
  }

  // Check directory metadata is the same
  bool is_same = false;

  // Check file metadata is the same
  if(m_is_retry_strict) {
    // strict checking -- all metadata must match
    if(file.id()               == remote_file.fmd().id()               &&
       file.cont_id()          == remote_file.fmd().cont_id()          &&
       file.uid()              == remote_file.fmd().uid()              &&
       file.gid()              == remote_file.fmd().gid()              &&
       file.size()             == remote_file.fmd().size()             &&
       file.layout_id()        == remote_file.fmd().layout_id()        &&
       file.flags()            == remote_file.fmd().flags()            &&
       file.checksum().type()  == remote_file.fmd().checksum().type()  &&
       file.checksum().value() == remote_file.fmd().checksum().value() &&
       file.ctime().sec()      == remote_file.fmd().ctime().sec()      &&
       file.mtime().sec()      == remote_file.fmd().mtime().sec()      &&
       file.name()             == remote_file.fmd().name()             &&
       file.path()             == remote_file.fmd().path())
         is_same = true;
  } else {
    // not strict checking -- only file-invariant metadata must match
    if(file.id()               == remote_file.fmd().id()               &&
       file.cont_id()          == remote_file.fmd().cont_id()          &&
       file.size()             == remote_file.fmd().size()             &&
       file.layout_id()        == remote_file.fmd().layout_id()        &&
       file.checksum().type()  == remote_file.fmd().checksum().type()  &&
       file.checksum().value() == remote_file.fmd().checksum().value() &&
       file.mtime().sec()      == remote_file.fmd().mtime().sec()      &&
       file.name()             == remote_file.fmd().name()             &&
       file.path()             == remote_file.fmd().path())
         is_same = true;
  }

  if(!is_same) {
    std::cerr << file.path() << ": ";
    if((file.path()) != remote_file.fmd().path()) {
      std::cerr << "[EOS: " << remote_file.fmd().path() << "] ";
    }
    if(file.name() != remote_file.fmd().name()) {
      std::cerr << "name [DB: " << file.name() << " EOS: " << remote_file.fmd().name() << "] ";
    }
    if(file.id() != remote_file.fmd().id()) {
      std::cerr << "id [DB: " << file.id() << " EOS: " << remote_file.fmd().id() << "] ";
    }
    if(file.cont_id() != remote_file.fmd().cont_id()) {
      std::cerr << "id [DB: " << file.cont_id() << " EOS: " << remote_file.fmd().cont_id() << "] ";
    }
    if(file.uid() != remote_file.fmd().uid()) {
      std::cerr << "uid [DB: " << file.uid() << " EOS: " << remote_file.fmd().uid() << "] ";
    }
    if(file.gid() != remote_file.fmd().gid()) {
      std::cerr << "gid [DB: " << file.gid() << " EOS: " << remote_file.fmd().gid()  << "] ";
    }
    if(file.size() != remote_file.fmd().size()) {
      std::cerr << "size [DB: " << file.size() << " EOS: " << remote_file.fmd().size()  << "] ";
    }
    if(file.layout_id() != remote_file.fmd().layout_id()) {
      std::cerr << "layout_id [DB: " << file.layout_id() << " EOS: " << remote_file.fmd().layout_id()  << "] ";
    }
    if(file.flags() != remote_file.fmd().flags()) {
      std::cerr << "flags [DB: " << file.flags() << " EOS: " << remote_file.fmd().flags() << "] ";
    }
    if(file.checksum().type() != remote_file.fmd().checksum().type()) {
      std::cerr << "checksum_type [DB: " << file.checksum().type() << " EOS: " << remote_file.fmd().checksum().type() << "] ";
    }
    if(file.checksum().value() != remote_file.fmd().checksum().value()) {
      std::cerr << "checksum_value [DB: " << checksum::ChecksumBlob::ByteArrayToHex(file.checksum().value())
                << " EOS: "               << checksum::ChecksumBlob::ByteArrayToHex(remote_file.fmd().checksum().value()) << "] ";
    }
    if(file.ctime().sec() != remote_file.fmd().ctime().sec()) {
      std::cerr << "ctime [DB: " << file.ctime().sec() << " EOS: " << remote_file.fmd().ctime().sec() << "] ";
    }
    if(file.mtime().sec() != remote_file.fmd().mtime().sec()) {
      std::cerr << "mtime [DB: " << file.mtime().sec() << " EOS: " << remote_file.fmd().mtime().sec() << "] ";
    }
    std::cerr << std::endl;
    return false;
  }

  // Check xattrs
  for(const auto &xattr : file.xattrs()) {
    auto xattr_it = remote_file.fmd().xattrs().find(xattr.first);
    if(xattr_it == remote_file.fmd().xattrs().end() || xattr.second != xattr_it->second) {
      std::cerr << "file " << file.id() << ": xattrs do not match" << std::endl;
      return false;
    }
  }

  return true;
}


unsigned int EosImportFiles::pruneExistingFiles()
{
  std::vector<eos::rpc::FileMdProto> filesToCheck;
  filesToCheck.swap(m_files);

  for(auto &file : filesToCheck) {
    if(compareMD(file)) {
      m_files.push_back(file);
    }
  }

  return filesToCheck.size() - m_files.size();  
}


void EosImportFiles::processFiles()
{
  while(true) {
    // Check for exceptions thrown by the other thread
    if(m_exptr != nullptr) {
      std::rethrow_exception(m_exptr);
    }

    auto files = getNextBatch();
    if(files.empty()) break;

    // Put results on stdout for debugging
    if(m_is_json) {
      char delim = '[';
      for(auto &file : files) {
        std::cout << delim << XrdSsiPb::Log::DumpProtobuf(&file);
        delim = ',';
      }
      if(delim == ',') std::cout << "]";
    }

    // Pass files to gRPC injection thread
    m_files_mutex.lock();
    m_files.insert(m_files.end(), files.begin(), files.end());
    m_files_mutex.unlock();
  }
}


std::vector<eos::rpc::FileMdProto> EosImportFiles::getNextBatch()
{
  std::vector<eos::rpc::FileMdProto> files;

  if(m_selectFilesDbRset.isEmpty()) return files;

  for(unsigned int b = 0; b < m_batch_size && m_selectFilesDbRset.next(); ++b) {
    eos::rpc::FileMdProto file;

    file.set_id(m_selectFilesDbRset.columnUint64("FILEID"));
    file.set_cont_id(m_selectFilesDbRset.columnUint64("PARENT_FILEID"));
    file.set_uid(m_selectFilesDbRset.columnUint64("DISK_UID"));
    file.set_gid(m_selectFilesDbRset.columnUint64("DISK_GID"));
    file.set_size(m_selectFilesDbRset.columnUint64("FILESIZE"));
    file.set_layout_id(m_layout);
    // Filemode: filter out S_ISUID, S_ISGID and S_ISVTX because EOS does not follow POSIX semantics for these bits
    auto filemode = m_selectFilesDbRset.columnUint64("FILEMODE");
    filemode &= !(S_ISUID | S_ISGID | S_ISVTX);
    file.set_flags(filemode);
    file.mutable_checksum()->set_type("adler");
    file.mutable_checksum()->set_value(convertChecksum(m_selectFilesDbRset.columnUint64("CHECKSUM")));

    // Timestamps
    file.mutable_ctime()->set_sec(m_selectFilesDbRset.columnUint64("CTIME"));
    file.mutable_mtime()->set_sec(m_selectFilesDbRset.columnUint64("MTIME"));
    // we don't care about file.stime (sync time, used for CERNBox)
    // BTIME is set as an extended attribute (see below)

    // Filename and path
    auto p = eos::client::manglePathname(m_castor_prefix, m_eos_prefix, m_selectFilesDbRset.columnString("PATH"), m_selectFilesDbRset.columnString("FILENAME"));
    file.set_name(p.basename);
    file.set_path(p.pathname);
    // we don't care about link_name

    // Extended attributes:
    //
    // 1. Archive File ID
    std::string archiveId(std::to_string(file.id()));
    file.mutable_xattrs()->insert(google::protobuf::MapPair<std::string,std::string>("sys.archive.file_id", archiveId));
    // 2. Storage Class
    auto sc = m_storageClass.at(m_selectFilesDbRset.columnUint64("CLASSID"));
    file.mutable_xattrs()->insert(google::protobuf::MapPair<std::string,std::string>("sys.archive.storage_class", sc));
    // 3. Birth Time
    // POSIX ATIME (Access Time) is used by CASTOR to store the file creation time. EOS calls this "birth time",
    // but there is no place in the namespace to store it, so it is stored as an extended attribute.
    auto btime = m_selectFilesDbRset.columnUint64("BTIME");
    file.mutable_xattrs()->insert(google::protobuf::MapPair<std::string,std::string>("eos.btime", std::to_string(btime)));

    // Indicate that there is a tape-resident replica of this file (except for zero-length files)
    if(file.size() > 0) {
      file.mutable_locations()->Add(65535);
    }
    // we don't care about unlink_locations (placeholder for files scheduled for deletion)

    files.push_back(file);
    ++m_total_files;
  }
  return files;
}


void EosImportFiles::grpcInject()
{
  unsigned int pruned_files;

  while(true) {
    std::lock_guard<std::mutex> files_lock(m_files_mutex);

    if(m_files.empty()) {
      if(m_files_done) return;
      continue;
    }

    // Skip rows
    if(m_skip > m_files.size()) {
      m_skip -= m_files.size();
      m_total_files += m_files.size();
      auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_start_time);
      std::cerr << "Skipped " << m_files.size() << " files in " << elapsed_time.count() << "s" << std::endl;
      m_files.clear();
      continue;
    } else if (m_skip > 0) {
      m_files.erase(m_files.begin(), m_files.begin() + m_skip);
      m_total_files += m_skip;
      auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_start_time);
      std::cerr << "Skipped " << m_skip << " files in " << elapsed_time.count() << "s" << std::endl;
      m_skip = 0;
    }

    // Remove files that already exist in the namespace
    if(m_is_check) {
      pruned_files = pruneExistingFiles();
      m_total_files += pruned_files;
    }

    try {
      // Inject files into EOS
      int num_errors = 0;
      eos::rpc::InsertReply replies;
      if(m_is_dry_run) {
        for(size_t i = 0; i < m_files.size(); ++i) {
          replies.add_retc(EPERM);
          replies.add_message("Dry run enabled, import into EOS suppressed");
        }
        num_errors = m_files.size();
      } else {
        num_errors = m_eosgrpc->FileInsert(m_files, replies);
      }

      auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_start_time);
      std::cerr << "Processed " << m_total_files << " files in " << elapsed_time.count() << "s (";
      if(pruned_files > 0) std::cerr << pruned_files << " files already existed, ";
      std::cerr << num_errors << " failures)" << std::endl;

      // Save errors for later processing
      if(m_is_retry || num_errors > 0) {
        saveFailedFiles(m_files, replies);
      }
    } catch(...) {
      // Pass exceptions to main thread for handling
      m_exptr = std::current_exception();
      return;
    }

    m_files.clear();
  }
}


std::string EosImportFiles::convertChecksum(uint32_t adler32)
{
  char bytes[4];
  for(int i = 3; i >= 0; --i, adler32 >>= 8) { bytes[i] = adler32 & 0xFF; }
  return std::string(bytes, 4);
}


uint32_t EosImportFiles::convertChecksum(const std::string &bytes)
{
  uint32_t adler32 = 0;
  for(int i = 0; i < 4; ++i) {
    adler32 <<= 8;
    adler32 += static_cast<unsigned char>(bytes[i]);
  }
  return adler32;
}

}} // namespace cta::migration
