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

#pragma once

#include <TestApi.hpp>

class TestClientCmd
{
public:
   TestClientCmd(int argc, const char *const *const argv);

   /*!
    * Send the protocol buffer across the XRootD SSI transport
    *
    * Synchronous case: for metadata-only requests
    */
   void Send();

   /*!
    * Send the protocol buffer across the XRootD SSI transport
    *
    * Asynchronous case: for data and stream requests
    */
   void SendAsync();

private:
   /*!
    * Throw an exception with usage help
    */
   void throwUsage(const std::string &error_txt = "") const;

   /*
    * Member variables
    */
   std::unique_ptr<XrdSsiPbServiceType> m_test_service_ptr;    //!< Pointer to Service object
   std::string                          m_execname;            //!< Executable name of this program
   std::string                          m_endpoint;            //!< hostname:port of XRootD server
   test::Request                        m_request;             //!< Protocol Buffer for the command to send to the server
   int                                  m_repeat;              //!< No. of times to repeat command over a single connection
};

