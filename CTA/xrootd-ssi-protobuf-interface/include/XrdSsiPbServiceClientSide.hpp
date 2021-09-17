/*!
 * @project        XRootD SSI/Protocol Buffer Interface Project
 * @brief          XRootD SSI client-side Service object management
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

#include <XrdSsi/XrdSsiProvider.hh>
#include <XrdSsi/XrdSsiService.hh>
#include "XrdSsiPbConfig.hpp"
#include "XrdSsiPbException.hpp"
#include "XrdSsiPbRequest.hpp"



//! XrdSsiProviderClient is instantiated and managed by the SSI library
extern XrdSsiProvider *XrdSsiProviderClient;



namespace XrdSsiPb {

// Constants

//! Default log level for clients
const std::string DefaultLogLevel = "error";

//! Default size for the response buffer in bytes
const unsigned int DefaultResponseBufferSize = 16384;

/*!
 * Wrapper class to manage the XRootD SSI service on the client side
 */
template <typename RequestType, typename MetadataType, typename DataType, typename AlertType>
class ServiceClientSide
{
public:
   //! Class to keep data request pointers and futures together for clean-up
   class DataFuture
   {
   public:
      DataFuture(Request<RequestType,MetadataType,DataType,AlertType> *request_ptr) :
         m_request_ptr(request_ptr) {}

      void wait() {
         m_request_ptr->GetDataFuture().get();
         delete m_request_ptr;
      }

      void delete_request() {
         delete m_request_ptr;
      }

   private:
      Request<RequestType,MetadataType,DataType,AlertType> *m_request_ptr;
   };

   //! Construct and provide additional configuration options in a configuration object
   ServiceClientSide(std::string endpoint, const std::string &resource, const Config &config);

   //! Construct with default configuration options
   ServiceClientSide(const std::string &endpoint, const std::string &resource) :
      ServiceClientSide(endpoint, resource, Config()) {}

   /*!
    * Construct from a configuration object only.
    *
    * The endpoint and resource must be set in the Config object
    */
   ServiceClientSide(const Config &config) :
      ServiceClientSide("", "", config) {}

   //! Destructor
   virtual ~ServiceClientSide();

   /*!
    * Send a Request and receive a synchronous Metadata-only Response.
    *
    * This version should be used for Requests which have a Metadata-only response. The Request object
    * is automatically garbage collected.
    *
    * @param[in]     request
    * @param[out]    response
    */
   void Send(const RequestType &request, MetadataType &response) {
      auto data_future(SendAsync(request, response));
      data_future.wait();
   }

   /*!
    * Send a Request, receive a synchronous Metadata Response and a future for an asynchronous Data/Stream response.
    *
    * This version should be used for Requests which have a Data/Stream response. A pointer to the Request
    * object is returned to the caller, and it is the caller's responsibility to delete this after the
    * promise has been fulfilled.
    *
    * @param[in]     request
    * @param[out]    response
    *
    * @returns       future for Data/Stream requests. This return value can be ignored for Metadata-only Responses.
    */
   DataFuture SendAsync(const RequestType &request, MetadataType &response);

private:
   std::string m_endpoint;                           //!< hostname:port of the XRootD server
   XrdSsiResource m_resource;                        //!< Requests are bound to this resource. As the resource is
                                                     //!< reusable, the lifetime of the resource is the same as the
                                                     //!< lifetime of the Service object.

   XrdSsiService *m_server_ptr;                      //!< Pointer to XRootD Server object

   unsigned int m_response_bufsize;                  //!< Buffer size for responses from the XRootD SSI server

   static constexpr const char* const LOG_SUFFIX = "Pb::ServiceClientSide";    //!< Identifier for log messages
};



template <typename RequestType, typename MetadataType, typename DataType, typename AlertType>
ServiceClientSide<RequestType, MetadataType, DataType, AlertType>::
ServiceClientSide(std::string endpoint, const std::string &resource, const Config &config) :
   m_endpoint(endpoint),
   m_resource(resource)
{
   // If endpoint and resource are not provided as parameters, get them from the config

   if(m_endpoint.length() == 0) {
      auto ep = config.getOptionValueStr("endpoint");
      if(ep.first) {
         m_endpoint = ep.second;
      } else {
         throw XrdSsiException("Config error: endpoint missing");
      }
   }
   if(resource.length() == 0) {
      auto r = config.getOptionValueStr("resource");
      if(r.first) {
         m_resource = XrdSsiResource(r.second);
      } else {
         throw XrdSsiException("Config error: resource missing");
      }
   }

   /*
    * Get the Service pointer
    *
    * Note: Logging is not available until GetService() has been called
    */
   XrdSsiErrInfo eInfo;
   if(!(m_server_ptr = XrdSsiProviderClient->GetService(eInfo, m_endpoint)))
   {
      throw XrdSsiException(eInfo);
   }

   // Set configuration options

   /*
    * Set the log level
    *
    * The default is no logging. A different log level can be set using the XrdSsiPbLogLevel environment
    * variable. Logging is sent to the XRootD logger, which defaults to stderr.
    *
    * XrdSsiPbLogLevel should be set to a space-separated list of one or more of the following options:
    *    error warning info debug protobuf protoraw all
    */
   auto log_levels = config.getOptionList("log");
   if(!log_levels.empty()) {
      Log::SetLogLevel(log_levels);
   } else {
      Log::SetLogLevel(DefaultLogLevel);
   }
   auto hiRes = config.getOptionValueBool("log.hiRes");
   if(hiRes.first) {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Timestamp hiRes = ", hiRes.second ? "true" : "false");
      if(hiRes.second) Log::SetHiRes();
   }

   // Set the response buffer size for streaming responses (default: 1 Kb)
   auto response_bufsize = config.getOptionValueInt("response.bufsize");
   m_response_bufsize = response_bufsize.first ? response_bufsize.second : DefaultResponseBufferSize;

   /*
    * Set Resource options
    *
    * The following options are available (possibly other options will be added in future):
    *
    *    Reusable    Resource context may be cached and is reusable
    *
    * For details, see XRootD SSIv2 Reference, section 2.2, "Step 2: Define a Resource"
    */
   auto resource_option = config.getOptionList("resource.options");
   m_resource.rOpts = 0;
   for(auto &r_opt : resource_option) {
      if(r_opt == "Reusable") {
         Log::Msg(Log::DEBUG, LOG_SUFFIX, "Resource option Reusable = ON");
         m_resource.rOpts = XrdSsiResource::Reusable;
      } else {
         throw XrdSsiException("Config error: resource option " + r_opt + " is unrecognised");
      }
   }

   Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called ServiceClientSide constructor");
}



template <typename RequestType, typename MetadataType, typename DataType, typename AlertType>
ServiceClientSide<RequestType, MetadataType, DataType, AlertType>::~ServiceClientSide()
{
   Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called ~ServiceClientSide destructor");

   if(!m_server_ptr->Stop())
   {
      Log::Msg(Log::WARNING, LOG_SUFFIX, "ServiceClientSide object was destroyed before shutting down the Service, possible memory leak");
   }
}



template <typename RequestType, typename MetadataType, typename DataType, typename AlertType>
typename ServiceClientSide<RequestType, MetadataType, DataType, AlertType>::DataFuture
ServiceClientSide<RequestType, MetadataType, DataType, AlertType>::SendAsync(const RequestType &request, MetadataType &response)
{
   // Instantiate the Request object
   auto request_ptr = new Request<RequestType, MetadataType, DataType, AlertType>(request, m_response_bufsize);
   auto metadata_future = request_ptr->GetMetadataFuture();
   DataFuture data_future(request_ptr);

   // Log the request Protobuf
   Log::Msg(Log::PROTOBUF, LOG_SUFFIX, "Sending Request:");
   Log::DumpProtobuf(Log::PROTOBUF, &request);

   // Transfer ownership of the Request to the Service object.
   m_server_ptr->ProcessRequest(*request_ptr, m_resource);

   try {
      // Wait synchronously for the framework to return its Response (or an exception)
      response = metadata_future.get();
   } catch(XrdSsiException &ex) {
      // Something went wrong in the XRootD SSI framework. In any case, the framework should have
      // released the Request, so we should delete it here.
      data_future.delete_request();

      throw ex;
   }

   // Return the future for Data/Stream requests
   return data_future;
}

} // namespace XrdSsiPb
