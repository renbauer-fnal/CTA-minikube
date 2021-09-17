/*!
 * @project        XRootD SSI/Protocol Buffer Interface Project
 * @brief          XRootD SSI server-side Service object management
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

#include <map>

#include <XrdSsi/XrdSsiService.hh>
#include <XrdSsi/XrdSsiEntity.hh>
#include "XrdSsiPbRequestProc.hpp"
#include "XrdSsiPbLog.hpp"

namespace XrdSsiPb {

/*!
 * Service Object.
 *
 * Obtained using GetService() method of the XrdSsiPbServiceProvider factory
 */
template <typename RequestType, typename MetadataType, typename AlertType>
class Service : public XrdSsiService
{
public:
   Service() {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called Service() constructor");
   }
   virtual ~Service() {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called ~Service() destructor");
   }

#if 0
   /*!
    * Stop the Service.
    *
    * Requires some method to pass Finished(true) to in-flight Requests. This has been raised with Andy.
    * Currently not implemented as it would require tracking all in-flight Requests by the application
    * when this is really the job of the framework.
    */
   virtual bool Stop() override
   {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called Stop()");

      return false;
   }
#endif

   virtual void ProcessRequest(XrdSsiRequest &reqRef, XrdSsiResource &resRef) override;

   /*!
    * Perform Request pre-authorisation and/or resource optimisation.
    */
   virtual bool Prepare(XrdSsiErrInfo &eInfo, const XrdSsiResource &resource) override
   {
      const std::map<XrdSsiResource::Affinity, const char* const> ResourceMap = {
         { XrdSsiResource::None,    "None"    },
         { XrdSsiResource::Default, "Default" },
         { XrdSsiResource::Weak,    "Weak"    },
         { XrdSsiResource::Strong,  "Strong"  },
         { XrdSsiResource::Strict,  "Strict"  }
      };

      // Ensure there are no null pointers in the XrdSsiEntity object
      if(resource.client->name == nullptr) resource.client->name = "NULL";
      if(resource.client->host == nullptr) resource.client->host = "NULL";
      if(resource.client->vorg == nullptr) resource.client->vorg = "NULL";
      if(resource.client->role == nullptr) resource.client->role = "NULL";
      if(resource.client->grps == nullptr) resource.client->grps = "NULL";
      if(resource.client->endorsements == nullptr) resource.client->endorsements = "NULL";
      if(resource.client->creds == nullptr) resource.client->creds = "NULL";
      if(resource.client->tident == nullptr) resource.client->tident = "NULL";

      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Prepare():");
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "   Resource name: ", resource.rName);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "   Resource user: ", resource.rUser);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "   Resource info: ", resource.rInfo);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "   Hosts to avoid: ", resource.hAvoid);
      try {
         Log::Msg(Log::DEBUG, LOG_SUFFIX, "   Affinity: ", ResourceMap.at(resource.affinity));
      } catch (std::out_of_range &) {
         Log::Msg(Log::ERROR, LOG_SUFFIX, "   Affinity: INVALID ENUM VALUE (", resource.affinity, ")");
      }

      Log::Msg(Log::DEBUG, LOG_SUFFIX, "   Resource options: ",
             (resource.rOpts & XrdSsiResource::Reusable ? "Resuable " : ""),
             (resource.rOpts & XrdSsiResource::Discard  ? "Discard"   : ""));

      Log::Msg(Log::DEBUG, LOG_SUFFIX, "   Resource Client: ");
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Protocol:     ", resource.client->prot);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Name:         ", resource.client->name);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Host:         ", resource.client->host);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Vorg:         ", resource.client->vorg);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Role:         ", resource.client->role);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Grps:         ", resource.client->grps);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Endorsements: ", resource.client->endorsements);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Creds:        ", resource.client->creds);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Credslen:     ", resource.client->credslen);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Rsvd:         ", resource.client->rsvd);
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "      Tident:       ", resource.client->tident);

      return true;
   }

   /*!
    * Receive notification that a Request has been attached.
    *
    * This is required only if the service needs to make decisions on how to run a request based on whether
    * it is attached or detached. See Sect. 3.3.1 "Detached Requests" in the XRootD SSI documentation.
    */
   virtual bool Attach(XrdSsiErrInfo &eInfo, const std::string &handle,
                       XrdSsiRequest &reqRef, XrdSsiResource *resp) override
   {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called Attach()");

      return true;
   }

private:
   static constexpr const char* const LOG_SUFFIX = "Pb::Service";    //!< Identifier for log messages
};



/*!
 * Bind a Request to a Request Processor and execute the Processor.
 *
 * The client calls its ProcessRequest() method to hand off its Request and Resource objects. The client's
 * Request and Resource objects are transmitted to the server and passed into the service's ProcessRequest()
 * method.
 */
template <typename RequestType, typename MetadataType, typename AlertType>
void Service<RequestType, MetadataType, AlertType>::ProcessRequest(XrdSsiRequest &reqRef, XrdSsiResource &resRef)
{
   RequestProc<RequestType, MetadataType, AlertType> processor(resRef);

   // Bind the processor to the request. Inherits the BindRequest method from XrdSsiResponder.
   Log::Msg(Log::DEBUG, LOG_SUFFIX, "ProcessRequest(): Binding Processor to Request");
   processor.BindRequest(reqRef);

   // Execute the request, upon return the processor is deleted
   processor.Execute();

   // Tell the framework we have finished with the request object: unbind the request from the responder
   Log::Msg(Log::DEBUG, LOG_SUFFIX, "ProcessRequest(): Unbinding Processor from Request");
   processor.UnBindRequest();
}

} // namespace XrdSsiPb

