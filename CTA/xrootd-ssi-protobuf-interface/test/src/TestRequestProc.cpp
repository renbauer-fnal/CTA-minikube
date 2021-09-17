/*!
 * @project        XRootD SSI/Protocol Buffer Interface Project
 * @brief          XRootD SSI Responder class implementation
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
#include <XrdSsi/XrdSsiEntity.hh>
#include <XrdSsiPbLog.hpp>
#include <XrdSsiPbException.hpp>
#include <XrdSsiPbRequestProc.hpp>
#include "TestServiceProvider.hpp"
#include "TestStream.hpp"



/*
 * Class to process Request Messages
 */
class RequestMessage
{
public:
   RequestMessage(const XrdSsiEntity &client, const TestServiceProvider *service) {
      using namespace XrdSsiPb;

      Log::Msg(Log::DEBUG, LOG_SUFFIX, "RequestMessage() constructor: request received from client ",
               client.name, '@', client.host);
   }

   /*!
    * Process a Notification request or an Admin command request
    *
    * @param[in]     request
    * @param[out]    response        Response protocol buffer
    * @param[out]    stream          Reference to Response stream pointer
    */
   void process(const test::Request &request, test::Response &response, std::string &data_buffer, XrdSsiStream* &stream)
   {
      using namespace XrdSsiPb;

      // Artifical delay to simulate a heavier workload
      if(request.delay_ms() > 0) {
         Log::Msg(Log::DEBUG, LOG_SUFFIX, "process(): delay = ", request.delay_ms(), " ms");
         std::this_thread::sleep_for(std::chrono::milliseconds(request.delay_ms()));
      }

      switch(request.cmd()) {
         using namespace test;

         case Request::SEND_METADATA: {
            std::stringstream message;

            message << "Request payload:"                                                 << std::endl
                    << "bool   = "   << (request.record().test_bool() ? "true" : "false") << std::endl
                    << "int64  = "   <<  request.record().test_int64()                    << std::endl
                    << "double = "   <<  request.record().test_double()                   << std::endl
                    << "string = \"" <<  request.record().test_string() << "\""           << std::endl;

            response.set_message_txt(message.str());
            response.set_type(Response::RSP_SUCCESS);
            break;
         }
         case Request::SEND_DATA: {
            // Probably we should provide a function in the generic headers to make this simpler for the client.
            // As we now have to prepend the length, there is no advantage in using a string for the serialized
            // buffer. Probably better to change it to a char array managed using a unique pointer.

            // Return response header in metadata
            set_header(response);
            response.set_type(Response::RSP_SUCCESS);

            // Send data response
            test::Data data;

            // Round-robin test: set pointer in data object to record in request object
            data.set_allocated_record(const_cast<test::Record*>(&request.record()));
            Log::Msg(Log::PROTOBUF, LOG_SUFFIX, "process(): Set Data response:");
            Log::DumpProtobuf(Log::PROTOBUF, &data);
            data.SerializeToString(&data_buffer);
            
            // Prevent protobuf from trying to delete request record
            data.release_record();

            // Prepend length so Data and Streams can be handled the same way by the client
            char bufsize[sizeof(uint32_t)];
            google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(data_buffer.size(), reinterpret_cast<google::protobuf::uint8*>(bufsize));
            data_buffer.insert(0, bufsize, sizeof(uint32_t));
            break;
         }
         case Request::SEND_STREAM:
            // Create a XrdSsi stream object to return the results
            stream = new TestStream(request);

            // Return response header in metadata
            set_header(response);
            response.set_type(Response::RSP_SUCCESS);
            break;

         default:
            response.set_message_txt("Invalid cmd.");
            response.set_type(Response::RSP_ERR_PROTOBUF);
      }
   }

private:

   /*!
    * Set reply header in metadata
    */
   void set_header(test::Response &response) {
      const char* const TEXT_RED    = "\x1b[31;1m";
      const char* const TEXT_NORMAL = "\x1b[0m\n";

      std::stringstream header;

      header << TEXT_RED << "  Count "
                         << "    Int64 Value "
                         << "   Double Value "
                         << "Bool  String Value" << TEXT_NORMAL;

      response.set_message_txt(header.str());
   }

   static constexpr const char* const LOG_SUFFIX = "Pb::RequestMessage";    //!< Identifier for log messages
};



/*
 * Implementation of XRootD SSI subclasses
 */
namespace XrdSsiPb {

/*
 * Convert a framework exception into a Response
 */
template<>
void ExceptionHandler<test::Response, PbException>::operator()(test::Response &response, const PbException &ex)
{
   response.set_type(test::Response::RSP_ERR_PROTOBUF);
   response.set_message_txt(ex.what());
}



/*
 * Process the Notification Request
 */
template <>
void RequestProc<test::Request, test::Response, test::Alert>::ExecuteAction()
{
   try {
      // Perform a capability query on the XrdSsiProviderServer object: it must be a TestServiceProvider

      TestServiceProvider *test_service_ptr;
     
      if(!(test_service_ptr = dynamic_cast<TestServiceProvider*>(XrdSsiProviderServer)))
      {
         throw std::runtime_error("XRootD Service is not the Test Service");
      }

      RequestMessage request_msg(*(m_resource.client), test_service_ptr);
      request_msg.process(m_request, m_metadata, m_response_str, m_response_stream_ptr);
   } catch(PbException &ex) {
      m_metadata.set_type(test::Response::RSP_ERR_PROTOBUF);
      m_metadata.set_message_txt(ex.what());
   } catch(std::exception &ex) {
      // Serialize and send a log message

      test::Alert alert_msg;
      alert_msg.set_message_txt("Something bad happened");
      Alert(alert_msg);

      // Send the metadata response

      m_metadata.set_type(test::Response::RSP_ERR_SERVER);
      m_metadata.set_message_txt(ex.what());
   }
}

} // namespace XrdSsiPb
