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

#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <XrdSsiPbConfig.hpp>
#include <XrdSsiPbLog.hpp>
#include "GrpcClient.hpp"
#include "GrpcUtils.hpp"



namespace cta {
namespace migration {

class EosTestDirInject
{
public:
  EosTestDirInject(const std::string &configfile);

  void inject(const std::string &path, uint64_t fileid);

  std::string ping(const std::string &payload) {
    return m_eosgrpc->ping(payload);
  }

private:
  std::chrono::steady_clock::time_point m_start_time;    //!< Start the clock

  bool m_is_json;                                        //!< Display results in JSON format for debugging
  std::unique_ptr<eos::client::GrpcClient> m_eosgrpc;    //!< EOS gRPC API interface
  std::string m_castor_prefix;                           //!< CASTOR namespace prefix to strip
  std::string m_eos_prefix;                              //!< EOS namespace prefix to prepend

  std::map<unsigned int, std::string> m_storageClass;    //!< Mapping of CASTOR file class IDs to CTA storage classes
};


EosTestDirInject::EosTestDirInject(const std::string &configfile) :
  m_start_time(std::chrono::steady_clock::now())
{
  // Parse configuration file
  XrdSsiPb::Config config(configfile);

  auto is_json       = config.getOptionValueBool("castor.json");
  auto castor_prefix = config.getOptionValueStr("castor.prefix");
  auto eos_prefix    = config.getOptionValueStr("eos.prefix");
  auto endpoint      = config.getOptionValueStr("eos.endpoint");
  auto token         = config.getOptionValueStr("eos.token");

  // Connect to EOS
  m_eosgrpc = eos::client::GrpcClient::Create(endpoint.first ? endpoint.second : "localhost:50051", token.second);

  // Set parameters and defaults
  m_is_json       = is_json.first       ? is_json.second       : false;
  m_castor_prefix = castor_prefix.first ? castor_prefix.second : "/castor/cern.ch/";
  m_eos_prefix    = eos_prefix.first    ? eos_prefix.second    : "/eos/grpc/";
  // enforce a slash at beginning and end of prefixes
  eos::client::checkPrefix(m_castor_prefix);
  eos::client::checkPrefix(m_eos_prefix);

  // Get EOS namespace maximum container ID
  uint64_t eos_cid;
  uint64_t eos_fid;
  m_eosgrpc->GetCurrentIds(eos_cid, eos_fid);
}


void EosTestDirInject::inject(const std::string &path, uint64_t fileid)
{
  eos::rpc::ContainerMdProto dir;

  dir.set_id(fileid);
  // we don't care about dir.parent_id
  dir.set_uid(1000);
  dir.set_gid(1000);
  // we don't care about dir.tree_size
  dir.set_mode(0755);
  // we don't care about dir.flags

  // Timestamps
  dir.mutable_ctime()->set_sec(1553900400);
  dir.mutable_mtime()->set_sec(1553900400);
  // we don't care about dir.stime (sync time, used for CERNBox)

  // Directory name and full path
  auto dirname = eos::client::manglePathname(m_castor_prefix, m_eos_prefix, path);
  dir.set_name(dirname.basename);
  dir.set_path(dirname.pathname);

  // Extended attributes: Storage Class
  dir.mutable_xattrs()->insert(google::protobuf::MapPair<std::string,std::string>("sys.archive.storage_class", "my_storage_class"));

  std::vector<eos::rpc::ContainerMdProto> dirs;
  dirs.push_back(dir);

  // Put results on stdout for debugging
  if(m_is_json) {
    char delim = '[';
    for(auto &dir : dirs) {
      std::cout << delim << XrdSsiPb::Log::DumpProtobuf(&dir);
      delim = ',';
    }
    if(delim == ',') std::cout << "]";
  }

  // Inject directories into EOS
  eos::rpc::InsertReply replies;
  int num_errors = m_eosgrpc->ContainerInsert(dirs, replies);
  if(num_errors > 0) {
    throw std::runtime_error("EosTestDirInject::inject(): ContainerInsert failed with error " +
      std::to_string(replies.retc(0)) + ": " + replies.message(0));
  }

  auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_start_time);
  std::cerr << "Processed 1 directory in " << elapsed_time.count() << "s" << std::endl;
}

}} // namespace cta::migration


void throwUsage(const std::string &program, const std::string &error_txt)
{
  std::stringstream help;

  help << program << ": " << error_txt << std::endl
       << "Usage: " << program << " [--config <config_file>] [--fileid <fileid>] ping|--path <path>";

  throw std::runtime_error(help.str());
}


int main(int argc, const char* argv[])
{
  std::string configfile = "/etc/cta/castor-migration.conf";
  std::string path;
  uint64_t fileid = 0;

  bool doPing = false;

  try {
    for(auto i = 1; i < argc; ++i) {
      std::string option(argv[i]);

      if(option == "--config" && argc > ++i) {
        configfile = argv[i];
        continue;
      } else if(option == "--path" && argc > ++i) {
        path = argv[i];
        continue;
      } else if(option == "--fileid" && argc > ++i) {
        fileid = strtoul(argv[i], NULL, 0);
        continue;
      } else if(option == "ping") {
        doPing = true;
        continue;
      }
      throwUsage(argv[0], "invalid option " + option);
    }
    cta::migration::EosTestDirInject testDirInject(configfile);

    if(doPing) {
      std::cout << "Pinging EOS MGM using gRPC API..." << std::endl;
      auto pingStr = testDirInject.ping("Ping from EosTestDirInject");
      std::cout << "Ping successful. Server responded with: " << pingStr << std::endl;
    } else {
      if(path.empty()) throwUsage(argv[0], "path not specified");
      testDirInject.inject(path, fileid);
    }
  } catch(std::runtime_error &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
