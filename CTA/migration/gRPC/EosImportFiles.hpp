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

class EosImportFiles
{
public:
  EosImportFiles(const std::string &configfile, std::string storageClass = "", bool retry = false, bool check = false, uint64_t skip = 0);

  void preFlightCheck();
  void getStorageClasses();
  void listFailureModes();
  void listFailures();
  void forgetFailures();
  void select();
  void processFiles();
  void grpcInject();
  void setFilesDone() { m_files_done = true; }

private:
  void saveFailedFiles(const std::vector<eos::rpc::FileMdProto> &files, const eos::rpc::InsertReply &replies);
  bool compareMD(const eos::rpc::FileMdProto &file);
  unsigned int pruneExistingFiles();
  std::vector<eos::rpc::FileMdProto> getNextBatch();
  static std::string convertChecksum(uint32_t adler32);
  uint32_t convertChecksum(const std::string &bytes);

  std::chrono::steady_clock::time_point m_start_time;    //!< Start the clock
  unsigned int m_total_files;                            //!< Count how many have been processed

  bool m_is_json;                                        //!< Display results in JSON format for debugging
  bool m_is_dry_run;                                     //!< Pull the data from Oracle but don't inject into EOS
  bool m_is_retry;                                       //!< We are retrying failed jobs
  bool m_is_retry_strict;                                //!< Require all metadata to match during retries
  bool m_is_check;                                       //!< Check if each file exists in EOS namespace before injection
  uint64_t m_skip;                                       //!< Number of DB rows to skip before starting injection
  uint64_t m_layout;                                     //!< Disk layout to use
  std::string m_storageClassFilter;                      //!< File class/storage class to filter on
  std::string m_tableName;                               //!< Table containing list of files
  std::unique_ptr<rdbms::ConnPool> m_dbConnPool;         //!< Pool of Oracle database connections
  rdbms::Conn m_dbConn;                                  //!< The current database connection
  rdbms::Stmt m_selectFilesDbStmt;                       //!< The prepared statement for selecting files
  rdbms::Rset m_selectFilesDbRset;                       //!< The database cursor to the files result set
  unsigned int m_batch_size;                             //!< Number of records to fetch from the DB at a time

  std::unique_ptr<eos::client::GrpcClient> m_eosgrpc;    //!< EOS gRPC API interface
  std::string m_castor_prefix;                           //!< CASTOR namespace prefix to strip
  std::string m_eos_prefix;                              //!< EOS namespace prefix to prepend
  std::vector<eos::rpc::FileMdProto> m_files;            //!< List of directories to pass to injection thread
  std::mutex m_files_mutex;                              //!< Mutex to lock access to m_dirs
  std::atomic<bool> m_files_done;                        //!< Flag to indicate that all input has been processed

  std::map<unsigned int, std::string> m_storageClass;    //!< Mapping of CASTOR file class IDs to CTA storage classes
  std::map<std::string, std::string>  m_storageClassRev; //!< Reverse lookup of storage classes (for error handling)

  static std::exception_ptr m_exptr;                                //!< Pass exceptions from thread to main
  static const uint64_t EOS_FID_WARN             = 2500000000;      //!< Minimum value for EOS container ID
                                                                    //!< (CASTOR max fileid is currently ~1.8 billion)
  static constexpr const char* const TEXT_RED    = "\x1b[31;1m";    //!< Terminal formatting code for red text
  static constexpr const char* const TEXT_NORMAL = "\x1b[0m";       //!< Terminal formatting code for normal text
};

}} // namespace cta::migration
