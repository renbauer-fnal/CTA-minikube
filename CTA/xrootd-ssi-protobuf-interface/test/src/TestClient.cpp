/*!
 * @project        XRootD SSI/Protocol Buffer Interface Project
 * @brief          Command-line test client for XRootD SSI/Protocol Buffers
 * @copyright      Copyright 2018 CERN
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

#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <unistd.h>

#include <XrdSsiPbIStreamBuffer.hpp>
#include "TestClient.hpp"



bool is_metadata_done = false;



//
// Define XRootD SSI callbacks
//

namespace XrdSsiPb {

/*
 * Alert callback.
 *
 * Defines how Alert messages should be logged
 */
template<>
void RequestCallback<test::Alert>::operator()(const test::Alert &alert)
{
   Log::Msg(Log::INFO, LOG_SUFFIX, "Alert received:");

   // Output message in Json format
   Log::DumpProtobuf(Log::INFO, &alert);
}



/*
 * Data/Stream callback.
 *
 * Defines how incoming records from the stream should be handled
 */
template<>
void IStreamBuffer<test::Data>::DataCallback(test::Data record) const
{
   // Processing of Metadata and Data Responses are asynchronous. To guarantee that Metadata Responses
   // are fully processed before handing the Data Responses, we need to add some kind of synchronisation.
   // Probably something more portable than this for production code.
   while(!is_metadata_done) ;

   const test::Record &test_record = record.record();

   std::cout << std::setfill(' ') << std::setw(7)  << std::right << test_record.test_int32()  << ' '
             << std::setfill(' ') << std::setw(15) << std::right << test_record.test_int64()  << ' '
             << std::setfill(' ') << std::setw(15) << std::right << test_record.test_double() << ' '
             << (test_record.test_bool() ? "true " : "false")                                 << ' '
             << test_record.test_string() << std::endl;
}

} // namespace XrdSsiPb



//
// TestClient implementation
//

const std::string DEFAULT_ENDPOINT = "localhost:10956";

// Change console text colour

const char* const TEXT_RED    = "\x1b[31;1m";
const char* const TEXT_NORMAL = "\x1b[0m\n";



// Convert string to bool

bool to_bool(std::string str) {
   std::transform(str.begin(), str.end(), str.begin(), ::tolower);
   if(str == "true") return true;
   if(str == "false") return false;

   throw std::runtime_error(str + " is not a valid boolean value");
}



TestClientCmd::TestClientCmd(int argc, const char *const *const argv) :
   m_execname(argv[0]),
   m_endpoint(DEFAULT_ENDPOINT),
   m_repeat(1)
{
   // Strip path from execname

   size_t p = m_execname.find_last_of('/');
   if(p != std::string::npos) m_execname.erase(0, p+1);

   // Parse the command

   if(argc < 2) throwUsage("Missing command");

   std::string cmd(argv[1]);
   int next_arg = 2;

   if(cmd == "help") {
      throwUsage();
   } else if(cmd == "metadata") {
      m_request.set_cmd(test::Request::SEND_METADATA);
   } else if(cmd == "data") {
      m_request.set_cmd(test::Request::SEND_DATA);
   } else if(cmd == "stream") {
      if(argc < 3) throwUsage("Must specify number of records to return");
      m_request.set_cmd(test::Request::SEND_STREAM);
      m_request.set_repeat(strtol(argv[next_arg++], NULL, 0));
      if(m_request.repeat() < 1) throwUsage("Number of records to return must be a valid positive integer");
   } else {
      throwUsage("Unrecognized command: " + cmd);
   }

   // Parse command line options

   while(next_arg < argc) {
      std::string option(argv[next_arg++]);

      if(next_arg == argc) throwUsage("Unrecognised option or missing parameter");

      if(option == "--endpoint") {
         m_endpoint = argv[next_arg++];
      } else if(option == "--repeat") {
         m_repeat = std::atoi(argv[next_arg++]);
      } else if(option == "--bool") {
         m_request.mutable_record()->set_test_bool(to_bool(argv[next_arg++]));
      } else if(option == "--int64") {
         m_request.mutable_record()->set_test_int64(strtoll(argv[next_arg++], NULL, 0));
      } else if(option == "--double") {
         m_request.mutable_record()->set_test_double(strtod(argv[next_arg++], NULL));
      } else if(option == "--string") {
         m_request.mutable_record()->set_test_string(argv[next_arg++]);
      }
   }

   // Read environment variables

   XrdSsiPb::Config config;

   // If XRDDEBUG=1, switch on all logging
   if(getenv("XRDDEBUG")) {
      config.set("log", "all");
   }

   // XrdSsiPbLogLevel gives more fine-grained control over log level
   config.getEnv("log", "XrdSsiPbLogLevel");

   // If the server is down, we want an immediate failure. Set client retry to a single attempt.
   XrdSsiProviderClient->SetTimeout(XrdSsiProvider::connect_N, 1);

   // Obtain a Service Provider
   std::string resource("/test");
   m_test_service_ptr = std::make_unique<XrdSsiPbServiceType>(m_endpoint, resource, config);
}



void TestClientCmd::Send()
{
   if(m_request.cmd() == test::Request::SEND_METADATA) {
      try {
         // Send the Request to the Service and get a Response
         test::Response response;

         m_test_service_ptr->Send(m_request, response);

         // Handle responses
         switch(response.type())
         {
            using namespace test;
   
            case Response::RSP_SUCCESS:         std::cout << response.message_txt(); break;
            case Response::RSP_ERR_PROTOBUF:    throw XrdSsiPb::PbException(response.message_txt());
            case Response::RSP_ERR_USER:
            case Response::RSP_ERR_SERVER:      throw std::runtime_error(response.message_txt());
            default:                            throw XrdSsiPb::PbException("Invalid response type.");
         }
      } catch(XrdSsiPb::XrdSsiException &ex) {
         std::cerr << ex.what() << std::endl;
      }
   } else {
      SendAsync();
   }
}


void TestClientCmd::SendAsync()
{
   while(m_repeat-- > 0)
   {
      try {
         // Send the Request to the Service and get a Response
         test::Response response;

         XrdSsiPbServiceType::DataFuture stream_future(m_test_service_ptr->SendAsync(m_request, response));

         // Handle responses
         switch(response.type())
         {
            using namespace test;
   
            case Response::RSP_SUCCESS:         std::cout << response.message_txt(); break;
            case Response::RSP_ERR_PROTOBUF:    throw XrdSsiPb::PbException(response.message_txt());
            case Response::RSP_ERR_USER:
            case Response::RSP_ERR_SERVER:      throw std::runtime_error(response.message_txt());
            default:                            throw XrdSsiPb::PbException("Invalid response type.");
         }
         is_metadata_done = true;

         // If there is a Data/Stream payload, wait until it has been processed before exiting
         stream_future.wait();

      } catch(XrdSsiPb::XrdSsiException &ex) {
         // Don't stop for client-side XRootD errors, just report it and try again next time
         std::cerr << ex.what() << std::endl;
      }

      // Sleep for 5 secs between outgoing requests
      if(m_repeat > 0) sleep(5);
   }
}



void TestClientCmd::throwUsage(const std::string &error_txt) const
{
   std::stringstream help;

   if(error_txt != "") {
      help << m_execname << ": " << error_txt << std::endl << std::endl;
   }

   help << TEXT_RED << "XRootD SSI/Google Protocol Buffers 3 Test Client" << TEXT_NORMAL << std::endl
        << "Usage:" << std::endl
        << "  " << m_execname << " help                              Show this help message" << std::endl
        << "  " << m_execname << " metadata <payload>                Request a metadata-only response" << std::endl
        << "  " << m_execname << " data <payload>                    Request a simple data response" << std::endl
        << "  " << m_execname << " stream <num_records> <payload>    Request a stream response comprising the specified number of records" << std::endl << std::endl
        << "Where <payload> is specified with one or more of these options: " << std::endl
        << "  [--bool true|false] [--int64 <test_int64>] [--double <test_double>] [--string <test_string>]" << std::endl << std::endl
        << "Additional options:" << std::endl
        << "  [--endpoint <hostname>:<port>]    Address of the XRootD SSI server to connect to (default localhost:10955)" << std::endl
        << "  [--repeat <num>]                  Number of times to repeat the command using the same Service object (default 1)" << std::endl;

   throw std::runtime_error(help.str());
}



/*!
 * Start here
 *
 * @param    argc[in]    The number of command-line arguments
 * @param    argv[in]    The command-line arguments
 *
 * @retval 0    Success
 * @retval 1    The client threw an exception
 */
int main(int argc, const char **argv)
{
   try {
      // Test uninitialised logging : logging is not available until the test_service object is
      // instantiated, so this message should be silently ignored
      XrdSsiPb::Log::Msg(XrdSsiPb::Log::ERROR, "main", "Logging is not initialised");

      // Parse the command line arguments
      TestClientCmd cmd(argc, argv);

      // Send the protocol buffer
      cmd.Send();

      // Delete all global objects allocated by libprotobuf
      google::protobuf::ShutdownProtobufLibrary();

      return 0;
   } catch (XrdSsiPb::PbException &ex) {
      std::cerr << "Error in Google Protocol Buffers: " << ex.what() << std::endl;
   } catch (XrdSsiPb::XrdSsiException &ex) {
      std::cerr << "Error from XRootD SSI Framework: " << ex.what() << std::endl;
   } catch (std::runtime_error &ex) {
      std::cerr << ex.what() << std::endl;
   } catch (std::exception &ex) {
      std::cerr << "Caught exception: " << ex.what() << std::endl;
   } catch (...) {
      std::cerr << "Caught an unknown exception" << std::endl;
   }

   return 1;
}

