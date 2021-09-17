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
#include "EosImportDirs.hpp"
#include "GrpcUtils.hpp"


namespace cta {
namespace migration {

std::exception_ptr EosImportDirs::m_exptr = nullptr;


EosImportDirs::EosImportDirs(const std::string &configfile, std::string topLevelPath, bool delta, bool retry) :
  m_start_time(std::chrono::steady_clock::now()),
  m_total_dirs(0),
  m_is_retry(retry),
  m_topLevelPath(topLevelPath),
  m_dirs_done(false)
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

  // Determine which DB table to use
  if(retry) {
    m_tableName = "CTADIRSFAILED";
  } else if(delta) {
    m_tableName = "CTADELTADIRSHELPER";
  } else {
    m_tableName = "CTADIRSHELPER";
  }

  // Connect to Oracle
  if(!dbconn.first) {
    throw std::runtime_error("castor.db_login must be specified in the config file in the form oracle:user/password@TNS");
  }
  const auto dbLogin = rdbms::Login::parseString(dbconn.second);
  const uint64_t min_num_conns = 2;  // One connection for directory select and one for storage classes
  uint64_t actual_max_num_conns = min_num_conns;
  if(max_num_conns.first && ((uint64_t)max_num_conns.second) > min_num_conns) {
    actual_max_num_conns = max_num_conns.second;
  }
  m_dbConnPool = ::cta::make_unique<rdbms::ConnPool>(dbLogin, actual_max_num_conns);
  m_dbConn = m_dbConnPool->getConn();

  // Connect to EOS
  m_eosgrpc = eos::client::GrpcClient::Create(endpoint.first ? endpoint.second : "localhost:50051", token.second);

  // Set parameters and defaults
  m_is_json         = is_json.first       ? is_json.second       : false;
  m_is_dry_run      = is_dry_run.first    ? is_dry_run.second    : false;
  m_is_retry_strict = is_strict.first     ? is_strict.second     : true;
  m_batch_size      = batch_size.first    ? batch_size.second    : 10000;
  m_castor_prefix   = castor_prefix.first ? castor_prefix.second : "/castor/cern.ch/";
  m_eos_prefix      = eos_prefix.first    ? eos_prefix.second    : "/eos/grpc/";
  // enforce a slash at beginning and end of prefixes
  eos::client::checkPrefix(m_castor_prefix);
  eos::client::checkPrefix(m_eos_prefix);

  // Start up the gRPC injection thread
}


void EosImportDirs::select()
{
  const std::string sql =
    "SELECT "
    "  FILEID,"
    "  PATH,"
    "  DISK_UID,"
    "  DISK_GID,"
    "  FILEMODE,"
    "  CTIME,"
    "  MTIME,"
    "  CLASSID,"
    "  DEPTH "
    "FROM " + m_tableName + " "
    "WHERE "
    "  PATH LIKE :TOP_LEVEL_PATH "
    "ORDER BY "
    "  DEPTH ASC";

  m_selectDirsDbStmt = m_dbConn.createStmt(sql);
  auto topLevelPathPattern = m_topLevelPath + (m_topLevelPath.back() == '/' ? "%" : "/%");
  m_selectDirsDbStmt.bindString(":TOP_LEVEL_PATH", topLevelPathPattern);
  std::cerr << "Executing SELECT query against " << m_tableName << " table...";
  m_selectDirsDbRset = m_selectDirsDbStmt.executeQuery();
  std::cerr << "done." << std::endl;
}


void EosImportDirs::select(const std::list<std::string> &dirs)
{
  std::string sql =
    "SELECT "
    "  FILEID,"
    "  PATH,"
    "  DISK_UID,"
    "  DISK_GID,"
    "  FILEMODE,"
    "  CTIME,"
    "  MTIME,"
    "  CLASSID,"
    "  DEPTH "
    "FROM CTADIRSHELPER";
  std::string connector = " WHERE ";
  for(auto &dir : dirs) {
    sql += connector + "PATH='" + dir + "'";
    connector = " OR ";
  }
  sql += " ORDER BY DEPTH ASC";

  m_selectDirsDbStmt = m_dbConn.createStmt(sql);
  std::cerr << "Executing SELECT query against CTADIRSHELPER table...";
  m_selectDirsDbRset = m_selectDirsDbStmt.executeQuery();
  std::cerr << "done." << std::endl;
}


void EosImportDirs::saveFailedDirs(const std::vector<eos::rpc::ContainerMdProto> &dirs,
  const eos::rpc::InsertReply &replies)
{
  std::cerr << "Updating list of failed inserts...";

  auto dir_it = dirs.begin();
  auto retc_it = replies.retc().begin();
  auto message_it = replies.message().begin();

  // Put the set of dirs into a temporary table
  cta::rdbms::AutoRollback autoRollback(m_dbConn);
  m_dbConn.setAutocommitMode(rdbms::AutocommitMode::AUTOCOMMIT_OFF);

  // This could be optimised by doing a batch insert.
  for( ; retc_it != replies.retc().end(); ++dir_it, ++retc_it, ++message_it) {
    auto retc = *retc_it;

    // We only care about successes if we were reading from the failed table (because we need to
    // delete successful retries in this case)
    if(!m_is_retry && retc == 0) continue;

    // If the failure mode was "Attempted to create container with id=<id>, which already exists",
    // verify that the metadata in the EOS namespace is what we expect. If so, ignore the error and
    // treat it as a successful injection.
    if(m_is_retry && retc == EINVAL && compareMD(*dir_it)) retc = 0;

    const std::string sql_insert = "INSERT INTO CTADIRSFAILEDTEMP("
      "  FILEID,"
      "  PATH,"
      "  DISK_UID,"
      "  DISK_GID,"
      "  FILEMODE,"
      "  CTIME,"
      "  MTIME,"
      "  CLASSID,"
      "  DEPTH,"
      "  RETC,"
      "  MESSAGE) "
      "VALUES("
      "  :FILEID,"
      "  :PATH,"
      "  :DISK_UID,"
      "  :DISK_GID,"
      "  :FILEMODE,"
      "  :CTIME,"
      "  :MTIME,"
      "  :CLASSID,"
      "  :DEPTH, "
      "  :RETC, "
      "  :MESSAGE)";
    auto stmt = m_dbConn.createStmt(sql_insert);
    stmt.bindUint64(":FILEID", dir_it->id());
    // Remove EOS prefix as it will be added again when we retry
    stmt.bindString(":PATH", dir_it->path().substr(m_eos_prefix.length() - 1));
    stmt.bindUint64(":DISK_UID", dir_it->uid());
    stmt.bindUint64(":DISK_GID", dir_it->gid());
    stmt.bindUint64(":FILEMODE", dir_it->mode());
    stmt.bindUint64(":CTIME", dir_it->ctime().sec());
    stmt.bindUint64(":MTIME", dir_it->mtime().sec());
    stmt.bindUint64(":CLASSID",
      strtol(m_storageClassRev.at(dir_it->xattrs().at("sys.archive.storage_class")).c_str(), NULL, 10));
    // Calculate depth. Note that this may be different from the depth in the original table, because
    // the path prefix has changed. The exact value doesn't matter, we just need an ordinal value so
    // that directories are created top-down.
    stmt.bindUint64(":DEPTH", std::count(dir_it->path().begin(), dir_it->path().end(), '/'));
    stmt.bindUint64(":RETC", retc);
    stmt.bindString(":MESSAGE", message_it->empty() ? "-" : *message_it);
    stmt.executeNonQuery();
  }

  // Merge the temporary table into the failure table
  if(m_is_retry) {
    // In the case of a retry, just handle deletions and updates
    std::cerr << "deleting...";
    const std::string sql_delete = "DELETE FROM CTADIRSFAILED "
      "WHERE FILEID IN (SELECT FILEID FROM CTADIRSFAILEDTEMP WHERE RETC = 0)";
    auto stmt_delete = m_dbConn.createStmt(sql_delete);
    stmt_delete.executeNonQuery();

    std::cerr << "updating...";
    const std::string sql_update = "UPDATE CTADIRSFAILED A SET("
      "  A.PATH,"
      "  A.DISK_UID,"
      "  A.DISK_GID,"
      "  A.FILEMODE,"
      "  A.CTIME,"
      "  A.MTIME,"
      "  A.CLASSID,"
      "  A.DEPTH,"
      "  A.RETC,"
      "  A.MESSAGE) = ("
      "SELECT"
      "  B.PATH,"
      "  B.DISK_UID,"
      "  B.DISK_GID,"
      "  B.FILEMODE,"
      "  B.CTIME,"
      "  B.MTIME,"
      "  B.CLASSID,"
      "  B.DEPTH,"
      "  B.RETC,"
      "  B.MESSAGE "
      "FROM CTADIRSFAILEDTEMP B "
      "WHERE A.FILEID = B.FILEID) "
      "WHERE EXISTS ("
      "  SELECT 1"
      "  FROM CTADIRSFAILEDTEMP B "
      "  WHERE A.FILEID = B.FILEID)";
    m_dbConn.setAutocommitMode(rdbms::AutocommitMode::AUTOCOMMIT_ON);
    auto stmt_update = m_dbConn.createStmt(sql_update);
    stmt_update.executeNonQuery();
  } else {
    // If it's not a retry, just handle insertions
    std::cerr << "inserting...";
    const std::string sql_insert = "INSERT INTO CTADIRSFAILED ("
      "  FILEID,"
      "  PATH,"
      "  DISK_UID,"
      "  DISK_GID,"
      "  FILEMODE,"
      "  CTIME,"
      "  MTIME,"
      "  CLASSID,"
      "  DEPTH,"
      "  RETC,"
      "  MESSAGE) "
      "SELECT "
      "  FILEID,"
      "  PATH,"
      "  DISK_UID,"
      "  DISK_GID,"
      "  FILEMODE,"
      "  CTIME,"
      "  MTIME,"
      "  CLASSID,"
      "  DEPTH,"
      "  RETC,"
      "  MESSAGE "
      "FROM CTADIRSFAILEDTEMP "
      "WHERE RETC != 0";
    m_dbConn.setAutocommitMode(rdbms::AutocommitMode::AUTOCOMMIT_ON);
    auto stmt_insert = m_dbConn.createStmt(sql_insert);
    stmt_insert.executeNonQuery();
  }
  std::cerr << "done." << std::endl;
}


bool EosImportDirs::compareMD(const eos::rpc::ContainerMdProto &dir)
{
  auto remote_dir = m_eosgrpc->GetMD(eos::rpc::CONTAINER, dir.id(), dir.path());

  // Check directory metadata is the same
  bool is_same = false;

  if(m_is_retry_strict) {
    // strict checking -- all metadata must match
    if(dir.id()           == remote_dir.cmd().id()          &&
       dir.uid()          == remote_dir.cmd().uid()         &&
       dir.gid()          == remote_dir.cmd().gid()         &&
       dir.mode()         == remote_dir.cmd().mode()        &&
       dir.ctime().sec()  == remote_dir.cmd().ctime().sec() &&
       dir.name()         == remote_dir.cmd().name()        &&
       (dir.path() + '/') == remote_dir.cmd().path())
         is_same = true;
  } else {
    // not strict checking -- only file-invariant metadata must match
    if(dir.id()           == remote_dir.cmd().id()          &&
       dir.name()         == remote_dir.cmd().name()        &&
       (dir.path() + '/') == remote_dir.cmd().path())
         is_same = true;
  }

  if(!is_same) {
    std::cerr << dir.path() << "/: ";
    if((dir.path() + '/') != remote_dir.cmd().path()) {
      std::cerr << "[EOS: " << remote_dir.cmd().path() << "] ";
    }
    if(dir.name() != remote_dir.cmd().name()) {
      std::cerr << "name [DB: " << dir.name() << " EOS: " << remote_dir.cmd().name() << "] ";
    }
    if(dir.id() != remote_dir.cmd().id()) {
      std::cerr << "id [DB: " << dir.id() << " EOS: " << remote_dir.cmd().id() << "] ";
    }
    if(dir.uid() != remote_dir.cmd().uid()) {
      std::cerr << "uid [DB: " << dir.uid() << " EOS: " << remote_dir.cmd().uid() << "] ";
    }
    if(dir.gid() != remote_dir.cmd().gid()) {
      std::cerr << "gid [DB: " << dir.gid() << " EOS: " << remote_dir.cmd().gid()  << "] ";
    }
    if(dir.mode() != remote_dir.cmd().mode()) {
      std::cerr << "mode [DB: " << dir.mode() << " EOS: " << remote_dir.cmd().mode() << "] ";
    }
    if(dir.ctime().sec() != remote_dir.cmd().ctime().sec()) {
      std::cerr << "ctime [DB: " << dir.ctime().sec() << " EOS: " << remote_dir.cmd().ctime().sec() << "] ";
    }
    std::cerr << std::endl;
    return false;
  }

  // Check xattrs
  for(const auto &xattr : dir.xattrs()) {
    auto xattr_it = remote_dir.cmd().xattrs().find(xattr.first);
    if(xattr_it == remote_dir.cmd().xattrs().end() || xattr.second != xattr_it->second) {
      std::cerr << "file " << dir.id() << ": xattrs do not match" << std::endl;
      return false;
    }
  }

  return true;
}


void EosImportDirs::listFailureModes()
{
  const std::string sql = "SELECT "
    "RETC, "
    "REGEXP_REPLACE(REPLACE(MESSAGE, PATH, '<path>'), 'id=[0-9]*', 'id=<id>') AS MSG, "
    "COUNT(*) AS CNT "
    "FROM CTADIRSFAILED "
    "GROUP BY RETC, "
    "REGEXP_REPLACE(REPLACE(MESSAGE, PATH, '<path>'), 'id=[0-9]*', 'id=<id>') "
    "ORDER BY RETC, MSG";
  auto stmt = m_dbConn.createStmt(sql);
  auto rset = stmt.executeQuery();

  while(rset.next()) {
    std::cout << rset.columnString("MSG") << " (retc=" << rset.columnUint64("RETC")
              << ", count=" << rset.columnUint64("CNT") << ")" << std::endl;
  }
}


void EosImportDirs::listFailures()
{
  const std::string sql = "SELECT FILEID, PATH, RETC, MESSAGE FROM CTADIRSFAILED";
  auto stmt = m_dbConn.createStmt(sql);
  auto rset = stmt.executeQuery();

  while(rset.next()) {
    std::cout << rset.columnString("FILEID") << "  " << rset.columnString("PATH") << "  "
              << rset.columnString("MESSAGE") << " (retc=" << rset.columnUint64("RETC") << ")" << std::endl;
  }
}


void EosImportDirs::forgetFailures()
{
  auto topLevelPathPattern = m_topLevelPath + (m_topLevelPath.back() == '/' ? "%\'" : "/%\'");
  const std::string sql = "DELETE FROM CTADIRSFAILED WHERE PATH LIKE '" + topLevelPathPattern;
  auto stmt = m_dbConn.createStmt(sql);
  std::cerr << "Deleting from CTADIRSFAILED...";
  stmt.executeNonQuery();
  std::cerr << "done." << std::endl;
}


void EosImportDirs::preFlightCheck()
{
  std::cerr << "Pre-flight check...";

  // Check EOS namespace directory ID is set to a suitably high value
  std::cerr << "EOS namespace...";
  uint64_t eos_cid;
  uint64_t eos_fid;

  m_eosgrpc->GetCurrentIds(eos_cid, eos_fid);
  std::cerr << "(" << eos_cid << "," << eos_fid << ")...";
  if(eos_cid < EOS_CID_WARN) {
    std::cerr << std::endl << TEXT_RED
              << "WARNING: EOS current container ID < " << EOS_CID_WARN << TEXT_NORMAL << std::endl;
  }

  // Check that CTADIRSFAILED is empty
  if(!m_is_retry) {
    std::cerr << "DB...";
    const std::string sql_select = "SELECT COUNT(*) AS CNT FROM CTADIRSFAILED";
    auto stmt = m_dbConn.createStmt(sql_select);
    auto rset = stmt.executeQuery();
    rset.next();
    if(rset.columnUint64("CNT") != 0) {
      std::cerr << std::endl << TEXT_RED
                << "WARNING: CTADIRSFAILED is not empty. You should either:" << std::endl
                << "  1. Fix the problem(s) and retry : see --failure-modes, --list-failures and --retry options" << std::endl
                << "  2. Clear the saved failures : see --forget-failures option" << TEXT_NORMAL << std::endl;
    }
  }

  std::cerr << "done." << std::endl;
}


void EosImportDirs::checkRootDirExists()
{
  auto tld = eos::client::manglePathname(m_castor_prefix, m_eos_prefix, m_topLevelPath);
  auto tld_pb = m_eosgrpc->GetMD(eos::rpc::CONTAINER, 0, tld.pathname);

  // Root directory for the import exists, nothing more to do
  if(tld_pb.cmd().id() != 0) return;

  if(m_topLevelPath[0] != '/') {
    throw std::runtime_error("Parent directory for import must begin with /");
  }

  // Try to create the tree up to the root
  std::string path = m_topLevelPath;
  std::list<std::string> dirsToCreate;
  while(tld_pb.cmd().id() == 0) {
    // Strip off any trailing slashes
    while(!path.empty() && path[path.length()-1] == '/') {
      path.resize(path.length()-1);
    }
    if(path.empty()) break;
    dirsToCreate.push_front(path);
    path.resize(path.find_last_of('/'));
    tld_pb = m_eosgrpc->GetMD(eos::rpc::CONTAINER, 0, path);
  }
  std::cerr << "Creating parent directory " << m_topLevelPath << "..." << std::endl;
  select(dirsToCreate);
  processDirs();

  // Wait for the initial set of directories to be processed
  bool is_done = false;
  while(!is_done) {
    m_dirs_mutex.lock();
    is_done = m_dirs.empty();
    m_dirs_mutex.unlock();
    if(m_exptr != nullptr) {
      std::rethrow_exception(m_exptr);
    }
  }

  // Check the root directory was created
  tld_pb = m_eosgrpc->GetMD(eos::rpc::CONTAINER, 0, tld.pathname);
  if(tld_pb.cmd().id() == 0) {
    throw std::runtime_error("Could not create parent directory for import");
  }
}


void EosImportDirs::getStorageClasses()
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


void EosImportDirs::processDirs()
{
  while(true) {
    // Check for exceptions thrown by the other thread
    if(m_exptr != nullptr) {
      std::rethrow_exception(m_exptr);
    }

    auto dirs = getNextBatch();
    if(dirs.empty()) break;

    // Put results on stdout for debugging
    if(m_is_json) {
      char delim = '[';
      for(auto &dir : dirs) {
        std::cout << delim << XrdSsiPb::Log::DumpProtobuf(&dir);
        delim = ',';
      }
      if(delim == ',') std::cout << "]";
    }

    // Pass dirs to gRPC injection thread
    m_dirs_mutex.lock();
    m_dirs.insert(m_dirs.end(), dirs.begin(), dirs.end());
    m_dirs_mutex.unlock();
  }
}


std::vector<eos::rpc::ContainerMdProto> EosImportDirs::getNextBatch() {
  std::vector<eos::rpc::ContainerMdProto> dirs;

  if(m_selectDirsDbRset.isEmpty()) return dirs;

  for(unsigned int b = 0; b < m_batch_size && m_selectDirsDbRset.next(); ++b) {
    eos::rpc::ContainerMdProto dir;

    dir.set_id(m_selectDirsDbRset.columnUint64("FILEID"));
    // we don't care about dir.parent_id
    dir.set_uid(m_selectDirsDbRset.columnUint64("DISK_UID"));
    dir.set_gid(m_selectDirsDbRset.columnUint64("DISK_GID"));
    // we don't care about dir.tree_size
    // Filemode: filter out S_ISUID, S_ISGID and S_ISVTX because EOS does not follow POSIX semantics for these bits
    auto filemode = m_selectDirsDbRset.columnUint64("FILEMODE");
    filemode &= ~(S_ISUID | S_ISGID | S_ISVTX);
    dir.set_mode(filemode);
    // we don't care about dir.flags

    // Timestamps
    dir.mutable_ctime()->set_sec(m_selectDirsDbRset.columnUint64("CTIME"));
    dir.mutable_mtime()->set_sec(m_selectDirsDbRset.columnUint64("MTIME"));
    // we don't care about dir.stime (sync time, used for CERNBox)

    // Directory name and full path
    auto dirname = eos::client::manglePathname(m_castor_prefix, m_eos_prefix, m_selectDirsDbRset.columnString("PATH"));
    dir.set_name(dirname.basename);
    dir.set_path(dirname.pathname);

    // Extended attributes: Storage Class
    const auto sc = m_storageClass.at(m_selectDirsDbRset.columnUint64("CLASSID"));
    dir.mutable_xattrs()->insert(google::protobuf::MapPair<std::string,std::string>("sys.archive.storage_class", sc));

    dirs.push_back(dir);
    ++m_total_dirs;
  }

  return dirs;
}


void EosImportDirs::grpcInject() {
  while(true) {
    std::lock_guard<std::mutex> dirs_lock(m_dirs_mutex);

    if(m_dirs.empty()) {
      if(m_dirs_done) return;
      continue;
    }

    try {
      // Inject directories into EOS
      int num_errors = 0;
      eos::rpc::InsertReply replies;
      if(m_is_dry_run) {
        for(size_t i = 0; i < m_dirs.size(); ++i) {
          replies.add_retc(EPERM);
          replies.add_message("Dry run enabled, import into EOS suppressed");
        }
        num_errors = m_dirs.size();
      } else {
        num_errors = m_eosgrpc->ContainerInsert(m_dirs, replies);
      }

      auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_start_time);
      std::cerr << "Processed " << m_total_dirs << " directories in " << elapsed_time.count() << "s (" << num_errors << " failures)" << std::endl;

      // Save errors for later processing
      if(m_is_retry || num_errors > 0) {
        saveFailedDirs(m_dirs, replies);
      }
    } catch(...) {
      // Pass exceptions to main thread for handling
      m_exptr = std::current_exception();
      return;
    }

    m_dirs.clear();
  }
}

}} // namespace cta::migration
