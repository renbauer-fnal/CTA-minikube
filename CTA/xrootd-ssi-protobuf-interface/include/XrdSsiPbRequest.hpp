/*!
 * @project        XRootD SSI/Protocol Buffer Interface Project
 * @brief          XRootD SSI Request class
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

#include <XrdSsi/XrdSsiRequest.hh>
#include <XrdSsi/XrdSsiStream.hh>
#include "XrdSsiPbIStreamBuffer.hpp"
#include "XrdVersion.hh"

namespace XrdSsiPb {

/*!
 * Request Callback class
 *
 * The client should specialize on this class for the Alert Response type. This permits an arbitrary
 * number of Alert messages to be sent before each Response. These can be used for any purpose defined
 * by the client and server, for example writing messages to the client log.
 */
template<typename CallbackArg>
class RequestCallback
{
public:
   void operator()(const CallbackArg &arg);

private:
   static constexpr const char* const LOG_SUFFIX = "Pb::RequestCallback";    //!< Identifier for log messages
};



/*!
 * Request class
 */
template <typename RequestType, typename MetadataType, typename DataType, typename AlertType>
class Request : public XrdSsiRequest
{
public:
   Request(const RequestType &request, unsigned int response_bufsize);

   virtual ~Request() {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called ~Request() destructor");
   }

   /*!
    * The implementation of GetRequest() must create request data, save it in some manner, and provide
    * it to the framework.
    */
   virtual char *GetRequest(int &reqlen) override
   {
      reqlen = m_request_str.size();
      return const_cast<char*>(m_request_str.c_str());
   }

   /*
    * Optionally also define the RelRequestBuffer() method to clean up when the framework no longer
    * needs access to the data. The thread used to initiate a request may be the same one used in the
    * GetRequest() call.
    */

   virtual bool ProcessResponse(const XrdSsiErrInfo &eInfo, const XrdSsiRespInfo &rInfo) override;

#if XrdMajorVNUM( XrdVNUMBER ) > 4
   virtual void                   ProcessResponseData(const XrdSsiErrInfo &eInfo, char *response_bufptr, int response_buflen, bool is_last) override;
#else
   virtual XrdSsiRequest::PRD_Xeq ProcessResponseData(const XrdSsiErrInfo &eInfo, char *response_bufptr, int response_buflen, bool is_last) override;
#endif

   virtual void Alert(XrdSsiRespInfoMsg &alert_msg) override;

   /*!
    * Return the future associated with this object's Metadata promise
    */
   std::future<MetadataType> GetMetadataFuture() { return m_metadata_promise.get_future(); }

   /*!
    * Return the future associated with this object's Data/Stream promise
    */
   std::future<void> GetDataFuture() { return m_data_promise.get_future(); }

private:
   void ProcessResponseMetadata();

   std::string                 m_request_str;         //!< Request buffer
   MetadataType                m_response_metadata;   //!< Response metadata object
   std::unique_ptr<char[]>     m_response_buffer;     //!< Pointer to storage for Data responses
   char                       *m_response_bufptr;     //!< Pointer to the Response buffer
   int                         m_response_bufsize;    //!< Size of the Response buffer

   std::promise<MetadataType>  m_metadata_promise;    //!< Promise a reply of Metadata type
   std::promise<void>          m_data_promise;        //!< Promise a data or stream response

   IStreamBuffer<DataType>     m_istream_buffer;      //!< Input stream buffer object

   RequestCallback<AlertType>  AlertCallback;         //!< Callback for Alert messages

   static constexpr const char* const LOG_SUFFIX  = "Pb::Request";    //!< Identifier for log messages
};



/*!
 * Request constructor
 */
template<typename RequestType, typename MetadataType, typename DataType, typename AlertType>
Request<RequestType, MetadataType, DataType, AlertType>::
Request(const RequestType &request, unsigned int response_bufsize) :
   m_response_bufsize(response_bufsize),
   m_istream_buffer(response_bufsize)
{
   Log::Msg(Log::DEBUG, LOG_SUFFIX, "Request(): Response buffer size = ", m_response_bufsize, " bytes");

   // Serialize the Request
   if(!request.SerializeToString(&m_request_str))
   {
      throw PbException("request.SerializeToString() failed");
   }
}



/*!
 * Process Responses from the server
 *
 * Requests are sent to the server asynchronously via the service object. ProcessResponse() informs
 * the Request object on the client side if it completed or failed.
 */
template<typename RequestType, typename MetadataType, typename DataType, typename AlertType>
bool Request<RequestType, MetadataType, DataType, AlertType>::ProcessResponse(const XrdSsiErrInfo &eInfo, const XrdSsiRespInfo &rInfo)
{
   Log::Msg(Log::DEBUG, LOG_SUFFIX, "ProcessResponse(): response type = ", rInfo.State());

   try {
      switch(rInfo.rType) {

      // Data and Metadata responses

      case XrdSsiRespInfo::isData:
         // Process Metadata
         ProcessResponseMetadata();

         // Process Data
         if(rInfo.blen > 0)
         {
            // For Data responses, we need to allocate the buffer to receive the data
            m_response_buffer = std::unique_ptr<char[]>(new char[m_response_bufsize]);
            m_response_bufptr = m_response_buffer.get();

            // Process Data Response: copy one chunk of data into the buffer, then call ProcessResponseData()
            GetResponseData(m_response_bufptr, m_response_bufsize);
         } else {
            // Return control of the object to the calling thread and delete rInfo
            Finished();

            // Response is Metadata-only
            m_data_promise.set_value();

            // It is now safe to delete the Request object
         }
         break;

      // Stream response

      case XrdSsiRespInfo::isStream:
         // Process Metadata
         ProcessResponseMetadata();

         // For Stream responses, we need to allocate the buffer to receive the data
         m_response_buffer = std::unique_ptr<char[]>(new char[m_response_bufsize]);
         m_response_bufptr = m_response_buffer.get();

         // Process Stream Response: copy one chunk of data into the buffer, then call ProcessResponseData()
         GetResponseData(m_response_bufptr, m_response_bufsize);

         break;

      // Handle errors in the XRootD framework (e.g. no response from server)

      case XrdSsiRespInfo::isError:     throw XrdSsiException(eInfo);

      // To implement detached requests, add another callback type which saves the handle

      case XrdSsiRespInfo::isHandle:    throw XrdSsiException("Detached requests are not implemented.");

      // To implement file requests, add another callback type

      case XrdSsiRespInfo::isFile:      throw XrdSsiException("File requests are not implemented.");

      // Handle invalid responses

      case XrdSsiRespInfo::isNone:
      default:                          throw XrdSsiException("Invalid Response.");
      }
   } catch(std::exception &ex) {
      std::exception_ptr p(std::current_exception());

      try {
         // Use the exception to fulfil the promise
         m_metadata_promise.set_exception(p);
      } catch(std::future_error &f_ex) {
         Log::Msg(Log::ERROR, LOG_SUFFIX, "ProcessResponse(): ", f_ex.what());

         // Metadata promise is already fulfilled, so set a Data/Stream exception instead
         m_data_promise.set_exception(p);
      }

      Finished();
   }

   // Response was a valid Data or Stream object, set response metadata

   return true;
}



/*!
 * Process Response Metadata
 *
 * A Response can (optionally) contain Metadata. This can be used for simple responses (e.g. status
 * code, short message) or as the header for large asynchronous data transfers or streaming data.
 */
template<typename RequestType, typename MetadataType, typename DataType, typename AlertType>
void Request<RequestType, MetadataType, DataType, AlertType>::ProcessResponseMetadata()
{
   int metadata_len;
   const char *metadata_buffer = GetMetadata(metadata_len);
   Log::Msg(Log::DEBUG, LOG_SUFFIX, "ProcessResponseMetadata(): received ", metadata_len, " bytes");
   Log::DumpBuffer(Log::PROTORAW, metadata_buffer, metadata_len);

   // Deserialize the metadata

   MetadataType metadata;

   if(metadata.ParseFromArray(metadata_buffer, metadata_len))
   {
      Log::DumpProtobuf(Log::PROTOBUF, &metadata);

      m_metadata_promise.set_value(metadata);
   }
   else
   {
      throw PbException("metadata.ParseFromArray() failed");
   }
}



/*!
 * Process Response Data.
 *
 * Data Responses are implemented as a binary stream, which is received one chunk at a time.
 * The chunk size is defined when the Request object is instantiated (see m_response_bufsize).
 *
 * In this implementation, the data returned in the response buffer is record-based, where each
 * record is a protocol buffer of type DataType. The framework ensures that the client application
 * receives only complete records.
 *
 * An alternative implementation would be to return typeless blobs to the client application,
 * possibly with the format defined in the metadata. This would make sense for cases where the
 * data size is in excess of the chunk size. Currently there is no use case for this but
 * it could be added in future if required. A possible implementation would be to use type traits
 * on DataType to decide how it should be handled.
 *
 * ProcessResponseData() is called either by GetResponseData(), or asynchronously at any time for data
 * streams.
 *
 * @retval    PRD_Normal    The response was accepted for processing
 * @retval    PRD_Hold      The response could not be handled at this time. The callback will be placed
 *                          in a global hold queue and the thread will be released. The client is
 *                          responsible for calling the static method XrdSsiRequest::RestartDataResponse()
 *                          to restart processing responses (in FIFO order).
 */
template<typename RequestType, typename MetadataType, typename DataType, typename AlertType>
#if XrdMajorVNUM( XrdVNUMBER ) > 4
void                   Request<RequestType, MetadataType, DataType, AlertType>
#else
XrdSsiRequest::PRD_Xeq Request<RequestType, MetadataType, DataType, AlertType>
#endif
             ::ProcessResponseData(const XrdSsiErrInfo &eInfo, char *response_bufptr, int response_buflen, bool is_last)
{
   Log::Msg(Log::DEBUG, LOG_SUFFIX, "ProcessResponseData(): received ", response_buflen, " bytes");
   Log::DumpBuffer(Log::PROTORAW, response_bufptr, response_buflen);

   // The buffer length is set to -1 if an error occurred setting up the response
   if(response_buflen == -1)
   {
      // Report an error message and abort the stream operation
      Log::Msg(Log::ERROR, LOG_SUFFIX, "ProcessResponseData(): fatal error from XRootD framework\n", eInfo.Get());
      m_data_promise.set_value();
      Finished();
#if XrdMajorVNUM( XrdVNUMBER ) > 4
      return;
#else
      return XrdSsiRequest::PRD_Normal;
#endif
   }

   // The buffer length can be 0 if the response is metadata only
   if(response_buflen != 0)
   {
      // Push stream/data buffer onto the input stream for the client
      m_istream_buffer.Push(response_bufptr, response_buflen);
   }

   if(is_last) // No more data to come
   {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "ProcessResponseData(): done");

      // Clean up

      // Set the data promise
      m_data_promise.set_value();

      // If Request objects are uniform, we could re-use them instead of deleting them, to avoid the
      // overhead of repeated object creation. This would require a more complex Request factory. For
      // now we just delete.

      Finished();
   }
   else
   {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "ProcessResponseData(): request more response data");

      // If there is more data, get the next chunk
      GetResponseData(m_response_bufptr, m_response_bufsize);
   }

#if XrdMajorVNUM( XrdVNUMBER ) < 5
   return XrdSsiRequest::PRD_Normal;
#endif
}



/*!
 * Deserialize Alert messages and call the Alert callback
 */

template<typename RequestType, typename MetadataType, typename DataType, typename AlertType>
void Request<RequestType, MetadataType, DataType, AlertType>::Alert(XrdSsiRespInfoMsg &alert_msg)
{
   try {
      // Get the Alert

      int alert_len;
      char *alert_buffer = alert_msg.GetMsg(alert_len);

      // Deserialize the Alert

      AlertType alert;

      if(alert.ParseFromArray(alert_buffer, alert_len))
      {
         AlertCallback(alert);
      }
      else
      {
         throw PbException("alert.ParseFromArray() failed");
      }
   } catch(std::exception &ex) {
      Log::Msg(Log::ERROR, LOG_SUFFIX, "Alert() could not process the Alert message: ", ex.what());
   }

   // Recycle the message to free memory

   alert_msg.RecycleMsg();
}

} // namespace XrdSsiPb

