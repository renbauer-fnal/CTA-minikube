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

#include <thread>
#include <common/make_unique.hpp>
#include <rdbms/ConnPool.hpp>
#include "GrpcClient.hpp"

namespace cta {
namespace migration {

class EosImportDirs
{
public:
  EosImportDirs(const std::string &configfile, std::string topLevelPath = "/", bool delta = false, bool retry = false);

  void preFlightCheck();
  void checkRootDirExists();
  void getStorageClasses();
  void listFailureModes();
  void listFailures();
  void forgetFailures();
  void select();
  void processDirs();
  void grpcInject();
  void setDirsDone() { m_dirs_done = true; }

private:
  void select(const std::list<std::string> &dirs);
  void saveFailedDirs(const std::vector<eos::rpc::ContainerMdProto> &dirs, const eos::rpc::InsertReply &replies);
  bool compareMD(const eos::rpc::ContainerMdProto &dir);
  std::vector<eos::rpc::ContainerMdProto> getNextBatch();

  std::chrono::steady_clock::time_point m_start_time;    //!< Start the clock
  unsigned int m_total_dirs;                             //!< Count how many have been processed

  bool m_is_json;                                        //!< Display results in JSON format for debugging
  bool m_is_dry_run;                                     //!< Pull the data from Oracle but don't inject into EOS
  bool m_is_retry;                                       //!< Retry failed jobs
  bool m_is_retry_strict;                                //!< Require all metadata to match during retries
  std::string m_topLevelPath;                            //!< Top-level path to match in CASTOR namespace queries
  std::string m_tableName;                               //!< Table containing list of directories
  std::unique_ptr<rdbms::ConnPool> m_dbConnPool;         //!< Pool of database connections
  rdbms::Conn m_dbConn;                                  //!< The current database connection
  rdbms::Stmt m_selectDirsDbStmt;                        //!< The prepared statement for selecting directories
  rdbms::Rset m_selectDirsDbRset;                        //!< The database cursor to the directories result set
  unsigned int m_batch_size;                             //!< Number of records to fetch from the DB at a time

  std::unique_ptr<eos::client::GrpcClient> m_eosgrpc;    //!< EOS gRPC API interface
  std::string m_castor_prefix;                           //!< CASTOR namespace prefix to strip
  std::string m_eos_prefix;                              //!< EOS namespace prefix to prepend

  std::map<unsigned int, std::string> m_storageClass;    //!< Mapping of CASTOR file class IDs to CTA storage classes
  std::map<std::string, std::string>  m_storageClassRev; //!< Reverse lookup of storage classes (for error handling)
  std::vector<eos::rpc::ContainerMdProto> m_dirs;        //!< List of directories to pass to injection thread
  std::mutex m_dirs_mutex;                               //!< Mutex to lock access to m_dirs
  std::atomic<bool> m_dirs_done;                         //!< Flag to indicate that all input has been processed

  static std::exception_ptr m_exptr;                                //!< Pass exceptions from thread to main
  static const uint64_t EOS_CID_WARN             = 2500000000;      //!< Minimum value for EOS container ID
                                                                    //!< (CASTOR max fileid is currently ~1.8 billion)
  static constexpr const char* const TEXT_RED    = "\x1b[31;1m";    //!< Terminal formatting code for red text
  static constexpr const char* const TEXT_NORMAL = "\x1b[0m";       //!< Terminal formatting code for normal text
};

}} // namespace cta::migration
