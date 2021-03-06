/*!
 * @project        XRootD SSI/Protocol Buffer Interface Project
 * @brief          XRootD SSI Responder class template
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

#pragma once

#include <future>

#include <XrdSsi/XrdSsiResponder.hh>
#include <XrdSsi/XrdSsiResource.hh>
#include <XrdSsi/XrdSsiStream.hh>
#include "XrdSsiPbException.hpp"
#include "XrdSsiPbAlert.hpp"

namespace XrdSsiPb {

/*!
 * Exception Handler class.
 *
 * This is used to send framework exceptions back to the client. The client should specialize on this
 * class for the Response class.
 */

template<typename MetadataType, typename ExceptionType>
class ExceptionHandler
{
public:
   void operator()(MetadataType &response, const ExceptionType &ex);
};



/*!
 * Request Processing class.
 *
 * This is an agent object that the Service object creates for each Request that it receives. The Request
 * object will be bound to the XrdSsiResponder object via a call to XrdSsiResponder::BindRequest(). Once
 * the relationship is established, the XrdSsi framework keeps track of the Request object and manages
 * its lifetime.
 *
 * The XrdSsiResponder class contains the methods needed to interact with the Request object: get the
 * Request, release storage, send Alerts, and post a Response. It also knows how to safely interact with
 * the Request object, handling asynchronous requests such as cancellation, broken TCP connections, etc. 
 */

template <typename RequestType, typename MetadataType, typename AlertType>
class RequestProc : public XrdSsiResponder
{
public:
   RequestProc(XrdSsiResource &resource) :
      m_resource(resource),
      m_response_stream_ptr(nullptr) {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called RequestProc() constructor");
   }
   virtual ~RequestProc() {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called ~RequestProc() destructor");
   }

           void Execute();
   virtual void Finished(XrdSsiRequest &rqstR, const XrdSsiRespInfo &rInfo, bool cancel=false) override;

private:
   /*!
    * Encapsulate the Alert protocol buffer inside a XrdSsiRespInfoMsg object.
    *
    * Alert message objects are created on the heap with lifetime managed by the XrdSsiResponder class.
    */
   void Alert(const AlertType &alert)
   {
      XrdSsiResponder::Alert(*(new AlertMsg<AlertType>(alert)));
   }

   /*!
    * Handle bad protocol buffer Requests.
    *
    * This class should store the exception in the Response Protocol Buffer, which the framework will
    * send back to the client. The client needs to define the specialized version of this class.
    */
   ExceptionHandler<MetadataType, PbException> Throw;

   /*!
    * Execute action after deserialization of the Request Protocol Buffer.
    *
    * The client needs to define the specialized version of this method.
    */
   void ExecuteAction() {
      Log::Msg(Log::ERROR, LOG_SUFFIX, "Called default ExecuteAction()");
   }

   // Member variables

   const XrdSsiResource &m_resource;    //!< Resource associated with the Request
   std::promise<void>    m_promise;     //!< Promise that the Request has been processed

   /*
    * Protocol Buffer members
    *
    * The Serialized Metadata Response buffer needs to be a member variable as it must stay in scope
    * after calling RequestProc(), until Finished() is called.
    *
    * The maximum amount of metadata that may be sent is defined by XrdSsiResponder::MaxMetaDataSZ
    * constant member.
    */
   RequestType   m_request;                //!< Request object
   MetadataType  m_metadata;               //!< Metadata Response object
   std::string   m_metadata_str;           //!< Serialized Metadata Response buffer
   std::string   m_response_str;           //!< Serialized Data Response buffer
   XrdSsiStream *m_response_stream_ptr;    //!< Stream Response pointer

   static constexpr const char* const LOG_SUFFIX = "Pb::RequestProc";    //!< Identifier for log messages
};



template <typename RequestType, typename MetadataType, typename AlertType>
void RequestProc<RequestType, MetadataType, AlertType>::Execute()
{
   Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called Execute()");

   // Deserialize the Request

   int request_len;
   const char *request_buffer = GetRequest(request_len);

   Log::Msg(Log::DEBUG, LOG_SUFFIX, "RequestProc(): received ", request_len, " bytes");
   Log::DumpBuffer(Log::PROTORAW, request_buffer, request_len);

   if(m_request.ParseFromArray(request_buffer, request_len))
   {
      Log::DumpProtobuf(Log::PROTOBUF, &m_request);

      // Pass control from the framework to the application
      ExecuteAction();
   }
   else
   {
      // Pass an exception back to the client and continue processing

      Throw(m_metadata, PbException("m_request.ParseFromArray() failed"));
   }

   // Release the request buffer

   ReleaseRequestBuffer();

   // Serialize and send the Metadata

   Log::Msg(Log::PROTOBUF, LOG_SUFFIX, "RequestProc(): sending metadata:");
   Log::DumpProtobuf(Log::PROTOBUF, &m_metadata);
   if(!m_metadata.SerializeToString(&m_metadata_str))
   {
      throw PbException("m_metadata.SerializeToString() failed");
   }
   Log::DumpBuffer(Log::PROTORAW, m_metadata_str.c_str(), m_metadata_str.size());
   SetMetadata(m_metadata_str.c_str(), m_metadata_str.size());

   // Send the Response

   if(m_response_stream_ptr != nullptr)
   {
      // Stream Response
      SetResponse(m_response_stream_ptr);
   }
   else if(m_response_str.size() != 0)
   {
      // Data Response
      Log::Msg(Log::PROTORAW, LOG_SUFFIX, "RequestProc(): sending Data response:");
      Log::DumpBuffer(Log::PROTORAW, m_response_str.c_str(), m_response_str.size());
      SetResponse(m_response_str.c_str(), m_response_str.size());
   }
   else
   {
      // Metadata-only Response
      //
      // It is necessary to set a Response even for empty responses, otherwise Finished()
      // will not be called on the Request.
      SetNilResponse();
   }

   // Wait for the framework to call Finished()

   auto finished = m_promise.get_future();

   finished.wait();
}



/*!
 * Clean up the Request Processing object.
 *
 * This is called when the Request has been processed or cancelled.
 *
 * If required, you can create specialized versions of this method to handle cancellation/cleanup for
 * specific message types.
 */

template <typename RequestType, typename MetadataType, typename AlertType>
void RequestProc<RequestType, MetadataType, AlertType>::Finished(XrdSsiRequest &rqstR, const XrdSsiRespInfo &rInfo, bool cancel)
{
   Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called Finished()");

   if(cancel) {
      // Reclaim resources dedicated to the request and tell caller the request object can be reclaimed
      Log::Msg(Log::WARNING, LOG_SUFFIX, "Request timed out or was cancelled");
   } else {
      // Reclaim any allocated resources
   }

   // Delete the stream object (if there is one for this request)
   delete m_response_stream_ptr;

   // Tell Execute() that we have Finished
   m_promise.set_value();
}

} // namespace XrdSsiPb

