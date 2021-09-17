/*!
 * @project        XRootD SSI/Protocol Buffer Interface Project
 * @brief          Class to manage XRootD SSI alerts
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

#include <XrdSsi/XrdSsiRespInfo.hh>
#include "XrdSsiPbException.hpp"
#include "XrdSsiPbLog.hpp"

namespace XrdSsiPb {

/*!
 * Alert message class.
 *
 * The SSI framework enforces the following rules for Alerts:
 *
 * 1. Alerts are sent in the order posted
 * 2. All outstanding Alerts are sent before the final response is sent
 *    (i.e. before SetResponse() is called)
 * 3. Once a final response is posted, subsequent Alert messages are discarded
 * 4. If a request is cancelled, all pending Alerts are discarded
 */
template<typename AlertType>
class AlertMsg : public XrdSsiRespInfoMsg
{
public:
   AlertMsg(const AlertType &alert) : XrdSsiRespInfoMsg(nullptr, 0)
   {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called AlertMsg() constructor");

      // Serialize the Alert

      if(!alert.SerializeToString(&alert_str))
      {
         throw PbException("alert.SerializeToString() failed");
      }

      msgBuf = const_cast<char*>(alert_str.c_str());
      msgLen = alert_str.size();
   }

   ~AlertMsg() {
      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called ~AlertMsg() destructor");
   }

   /*!
    * Method called by the framework to clean up after the Alert has been sent or discarded
    */
   void RecycleMsg(bool sent=true) {
      Log::Msg(Log::INFO, LOG_SUFFIX, "RecycleMsg(): \"", alert_str, "\" was ", (sent ? "sent." : "not sent."));
      delete this;
   }

private:
   std::string alert_str;    //!< Alert string

   static constexpr const char* const LOG_SUFFIX = "Pb::AlertMsg";    //!< Identifier for log messages
};

} // namespace XrdSsiPb

