/*!
 * @project        XRootD SSI/Protocol Buffer Interface Project
 * @brief          XRootD SSI Stream response class implementation
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

#include "XrdSsiPbOStreamBuffer.hpp"
#include "test.pb.h"



/*!
 * Test response stream object
 */
class TestStream : public XrdSsiStream
{
public:
   TestStream(const test::Request &request) :
      XrdSsiStream(XrdSsiStream::isActive),
      m_repeat(request.repeat()),
      m_record(std::move(request.record()))
   {
      using namespace XrdSsiPb;

      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called TestStream() constructor");
   }

   virtual ~TestStream() {
      using namespace XrdSsiPb;

      Log::Msg(Log::DEBUG, LOG_SUFFIX, "Called ~TestStream() destructor");
   }

   /*!
    * Synchronously obtain data from an active stream
    *
    * Active streams can only exist on the server-side. This XRootD SSI Stream class is marked as an
    * active stream in the constructor.
    *
    * @param[out]       eInfo   The object to receive any error description.
    * @param[in,out]    dlen    input:  the optimal amount of data wanted (this is a hint)
    *                           output: the actual amount of data returned in the buffer.
    * @param[in,out]    last    input:  should be set to false.
    *                           output: if true it indicates that no more data remains to be returned
    *                                   either for this call or on the next call.
    *
    * @return    Pointer to the Buffer object that contains a pointer to the the data (see below). The
    *            buffer must be returned to the stream using Buffer::Recycle(). The next member is usable.
    * @retval    0    No more data remains or an error occurred:
    *                 last = true:  No more data remains.
    *                 last = false: A fatal error occurred, eRef has the reason.
    */
   virtual Buffer *GetBuff(XrdSsiErrInfo &eInfo, int &dlen, bool &last) {
      using namespace XrdSsiPb;

      Log::Msg(Log::INFO, LOG_SUFFIX, "GetBuff(): XrdSsi buffer fill request (", dlen, " bytes)");

      XrdSsiPb::OStreamBuffer<test::Data> *streambuf;

      try {
         if(m_repeat <= 0) {
            // Nothing more to send, close the stream
            last = true;
            return nullptr;
         }
         sleep(10);

         streambuf = new XrdSsiPb::OStreamBuffer<test::Data>(dlen);

         for(bool is_buffer_full = false; m_repeat > 0 && !is_buffer_full; --m_repeat)
         {
            test::Data data;

            // Increment record number
            m_record.set_test_int32(m_record.test_int32() + 1);

            // Set record pointer in data object
            data.set_allocated_record(&m_record);

            is_buffer_full = streambuf->Push(data);

            // Prevent protobuf from trying to delete m_record
            data.release_record();
         }
         dlen = streambuf->Size();

         Log::Msg(Log::INFO, LOG_SUFFIX, "GetBuff(): Returning buffer with ", dlen, " bytes of data.");
         Log::DumpBuffer(Log::PROTORAW, streambuf->data, dlen);
      } catch(std::exception &ex) {
         std::ostringstream errMsg;
         errMsg << __FUNCTION__ << " failed: " << ex.what();
         eInfo.Set(errMsg.str().c_str(), ECANCELED);
         delete streambuf;
      } catch(...) {
         std::ostringstream errMsg;
         errMsg << __FUNCTION__ << " failed: Caught an unknown exception";
         eInfo.Set(errMsg.str().c_str(), ECANCELED);
         delete streambuf;
      }
      return streambuf;
   }

private:
   int32_t         m_repeat;    //!< Number of records to send in the response
   test::Record    m_record;    //!< Request record for round-robin Response test

   static constexpr const char* const LOG_SUFFIX  = "TestStream";    //!< Identifier for log messages
};

