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

#include <XrdSsiPbConfig.hpp>
#include <XrdSsiPbLog.hpp>
#include "GrpcClient.hpp"
#include "GrpcUtils.hpp"



namespace cta {
namespace migration {

class EosTestFileInject
{
public:
  EosTestFileInject(const std::string &configfile);

  void getMD(uint64_t fileid, const std::string &path);

  void inject(const std::string &path, uint64_t fileid);

  std::string ping(const std::string &payload) {
    return m_eosgrpc->ping(payload);
  }

private:
  static std::string convertChecksum(uint32_t adler32);

  std::chrono::steady_clock::time_point m_start_time;    //!< Start the clock

  bool m_is_json;                                        //!< Display results in JSON format for debugging
  std::unique_ptr<eos::client::GrpcClient> m_eosgrpc;    //!< EOS gRPC API interface
  std::string m_castor_prefix;                           //!< CASTOR namespace prefix to strip
  std::string m_eos_prefix;                              //!< EOS namespace prefix to prepend
  uint64_t m_layout;                                     //!< Disk layout to use

  std::map<unsigned int, std::string> m_storageClass;    //!< Mapping of CASTOR file class IDs to CTA storage classes
};


EosTestFileInject::EosTestFileInject(const std::string &configfile) :
  m_start_time(std::chrono::steady_clock::now())
{
  // Parse configuration file
  XrdSsiPb::Config config(configfile);

  auto is_json       = config.getOptionValueBool("castor.json");
  auto castor_prefix = config.getOptionValueStr("castor.prefix");
  auto eos_prefix    = config.getOptionValueStr("eos.prefix");
  auto endpoint      = config.getOptionValueStr("eos.endpoint");
  auto token         = config.getOptionValueStr("eos.token");
  auto eos_layout    = config.getOptionValueInt("eos.layout");

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
  m_is_json       = is_json.first       ? is_json.second       : false;
  m_castor_prefix = castor_prefix.first ? castor_prefix.second : "/castor/cern.ch/";
  m_eos_prefix    = eos_prefix.first    ? eos_prefix.second    : "/eos/grpc/";
  m_layout        = eos_layout.first    ? eos_layout.second    : default_layout;

  // enforce a slash at beginning and end of prefixes
  eos::client::checkPrefix(m_castor_prefix);
  eos::client::checkPrefix(m_eos_prefix);

  // Get EOS namespace maximum file ID
  uint64_t eos_cid;
  uint64_t eos_fid;
  m_eosgrpc->GetCurrentIds(eos_cid, eos_fid);
}


void EosTestFileInject::getMD(uint64_t fileid, const std::string &path)
{
  auto file = m_eosgrpc->GetMD(eos::rpc::FILE, fileid, path, m_is_json);

  std::cout << "File ID:        " << file.fmd().id() << std::endl
            << "Container ID:   " << file.fmd().cont_id() << std::endl
            << "Filename:       " << file.fmd().name() << std::endl
            << "Path:           " << file.fmd().path() << std::endl
            << "UID:            " << file.fmd().uid() << std::endl
            << "GID:            " << file.fmd().gid() << std::endl
            << "Size:           " << file.fmd().size() << std::endl
            << "Layout ID:      " << file.fmd().layout_id() << std::endl
            << "Flags:          " << file.fmd().flags() << std::endl
            << "Checksum Type:  " << file.fmd().checksum().type() << std::endl
            << "Checksum Value: " << file.fmd().checksum().value() << std::endl
            << "CTime:          " << file.fmd().ctime().sec() << std::endl
            << "MTime:          " << file.fmd().mtime().sec() << std::endl;

  std::cout << std::endl << "Extended Attributes:" << std::endl;
  for(const auto &xattr : file.fmd().xattrs()) {
    std::cout << "    " << xattr.first << " = " << xattr.second << std::endl;
  }
}


void EosTestFileInject::inject(const std::string &path, uint64_t fileid)
{
  eos::rpc::FileMdProto file;

  file.set_id(fileid);
  // we don't care about file.cont_id
  file.set_uid(1000);
  file.set_gid(1000);
  file.set_size(42);
  file.set_layout_id(m_layout);

  file.set_flags(0755);
  file.mutable_checksum()->set_type("adler");
  file.mutable_checksum()->set_value(convertChecksum(0xaabbccdd));

  // Timestamps
  file.mutable_ctime()->set_sec(1553900400);
  file.mutable_mtime()->set_sec(1553900400);
  // we don't care about file.stime (sync time, used for CERNBox)
  // BTIME is set as an extended attribute (see below)

  // Filename and path
  auto p = eos::client::manglePathname(m_castor_prefix, m_eos_prefix, path);
  file.set_name(p.basename);
  file.set_path(p.pathname);
  // we don't care about link_name

  // Extended attributes:
  //
  // 1. Archive File ID
  std::string archiveId(std::to_string(file.id()));
  file.mutable_xattrs()->insert(google::protobuf::MapPair<std::string,std::string>("sys.archive.file_id", archiveId));
  // 2. Storage Class
  file.mutable_xattrs()->insert(google::protobuf::MapPair<std::string,std::string>("sys.archive.storage_class", "my_storage_class"));
  // 3. Birth Time
  // POSIX ATIME (Access Time) is used by CASTOR to store the file creation time. EOS calls this "birth time",
  // but there is no place in the namespace to store it, so it is stored as an extended attribute.
  file.mutable_xattrs()->insert(google::protobuf::MapPair<std::string,std::string>("eos.btime", "1553900400"));

  // Indicate that there is a tape-resident replica of this file (except for zero-length files)
  if(file.size() > 0) {
    file.mutable_locations()->Add(65535);
  }
  // we don't care about unlink_locations (placeholder for files scheduled for deletion)

  std::vector<eos::rpc::FileMdProto> files;
  files.push_back(file);

  // Put results on stdout for debugging
  if(m_is_json) {
    char delim = '[';
    for(auto &file : files) {
      std::cout << delim << XrdSsiPb::Log::DumpProtobuf(&file);
      delim = ',';
    }
    if(delim == ',') std::cout << "]";
  }

  // Inject file into EOS
  eos::rpc::InsertReply replies;
  int num_errors = m_eosgrpc->FileInsert(files, replies);
  if(num_errors > 0) {
    throw std::runtime_error("EosTestFileInject::inject(): FileInsert failed with error " +
      std::to_string(replies.retc(0)) + ": " + replies.message(0));
  }

  auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_start_time);
  std::cerr << "Processed 1 file in " << elapsed_time.count() << "s" << std::endl;
}


std::string EosTestFileInject::convertChecksum(uint32_t adler32)
{
  char bytes[4];
  for(int i = 3; i >= 0; --i, adler32 >>= 8) { bytes[i] = adler32 & 0xFF; }
  return std::string(bytes, 4);
}

}} // namespace cta::migration


void throwUsage(const std::string &program, const std::string &error_txt)
{
  std::stringstream help;

  help << program << ": " << error_txt << std::endl
       << "Usage: " << program << " [--config <config_file>] [--fileid <fileid>] [--check] ping|--path <path>";

  throw std::runtime_error(help.str());
}


int main(int argc, const char* argv[])
{
  std::string configfile = "/etc/cta/castor-migration.conf";
  std::string path;
  uint64_t fileid = 0;

  bool doPing = false;
  bool doCheck = false;

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
      } else if(option == "--check") {
        doCheck = true;
        continue;
      } else if(option == "ping") {
        doPing = true;
        continue;
      }
      throwUsage(argv[0], "invalid option " + option);
    }
    cta::migration::EosTestFileInject testFileInject(configfile);

    if(doPing) {
      std::cout << "Pinging EOS MGM using gRPC API..." << std::endl;
      auto pingStr = testFileInject.ping("Ping from EosTestFileInject");
      std::cout << "Ping successful. Server responded with: " << pingStr << std::endl;
    } else if(doCheck) {
      testFileInject.getMD(fileid, path);
    } else {
      if(path.empty()) throwUsage(argv[0], "path not specified");
      testFileInject.inject(path, fileid);
    }
  } catch(std::runtime_error &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
