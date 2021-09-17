/*!
 * @project        XRootD SSI/Protocol Buffer Interface Project
 * @brief          Provides an interface to the XrdSsi log
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
#include <vector>
#include <sstream>
#include <iomanip>
#include <thread>

#include <XrdSys/XrdSysLogger.hh>
#include <XrdSys/XrdSysError.hh>
#include <google/protobuf/util/json_util.h>



namespace XrdSsi {
extern XrdSysError Log;     //!< XRootD Log object
}



namespace XrdSsiPb {
/*!
 * Interface to the XRootD logging object
 */
class Log
{
public:
   /*!
    * XRootD SSI log level bit values
    */
   enum LogLevel
   {
      // Low-order bits are defined by the XRootD framework
      NONE        = 0,
      XRD_LOGIN   = SYS_LOG_01,    //!< login and authentication
      XRD_DISC    = SYS_LOG_02,    //!< disconnect and unbind
      RESERVED_03 = SYS_LOG_03,
      RESERVED_04 = SYS_LOG_04,
      RESERVED_05 = SYS_LOG_05,
      RESERVED_06 = SYS_LOG_06,
      RESERVED_07 = SYS_LOG_07,
      RESERVED_08 = SYS_LOG_08,

      // High-order bits are used for our messages
      ERROR       = 1 << 16,       //!< Non-fatal error (fatal errors throw an exception)
      WARNING     = 1 << 17,       //!< Warning
      INFO        = 1 << 18,       //!< Information
      DEBUG       = 1 << 19,       //!< Debug information
      PROTOBUF    = 1 << 20,       //!< Show Protocol buffer contents
      PROTORAW    = 1 << 21        //!< Show raw Protocol buffer contents
   };

   /*!
    * The Log class has no instances; it is used for namespacing only.
    */
   Log() = delete;

   /*!
    * Set log level from an option list
    */
   static void SetLogLevel(const std::vector<std::string> &levels)
   {
      const std::map<std::string, uint32_t> option = {
         { "none",     NONE },
         { "error",    ERROR },
         { "warning",  ERROR | WARNING },
         { "info",     ERROR | WARNING | INFO },
         { "debug",    ERROR | WARNING | INFO | DEBUG },
         { "protobuf", PROTOBUF },
         { "protoraw", PROTORAW },
         { "all",      ERROR | WARNING | INFO | DEBUG | PROTOBUF | PROTORAW }
      };

      uint32_t loglevel = XrdSsi::Log.getMsgMask() & 0xFFFF;

      for(auto it = levels.begin(); it != levels.end(); ++it) {
         auto level = option.find(*it);
         if(level == option.end()) {
            Say("Ignoring unknown option ", *it);
         } else {
            loglevel |= level->second;
         }
      }

      XrdSsi::Log.setMsgMask(loglevel);
   }

   /*!
    * Set log level from a single string
    */
   static void SetLogLevel(const std::string &level)
   {
      std::vector<std::string> levels;

      levels.push_back(level);

      SetLogLevel(levels);
   }

   /*!
    * Enable microsecond resolution in timestamps
    */
   static void SetHiRes()
   {
      XrdSsi::Log.logger()->setHiRes();
   }

   /*!
    * Add a simple message to the XRootD log (no timestamp, identifiers or log level)
    */
   template<typename... Args>
   static void Say(Args... args)
   {
      // Bail out if the XRootD logging subsystem has not been initialised
      if(XrdSsi::Log.logger() == nullptr) return;

      std::stringstream logstream;

      BuildMessage(logstream, args...);

      XrdSsi::Log.Say(logstream.str().c_str());
   }

   /*!
    * Add a timestamped message to the XRootD log.
    *
    * Log messages take the form:
    *
    * <date> <time> <pid> ssi_SUFFIX: <message>
    *
    * @param[in] log_level    Log level of this message
    * @param[in] suffix       Log identifier SUFFIX (see above)
    * @param[in] Args         Variadic arguments which are concatenated to create the message
    */
   template<typename... Args>
   static void Msg(enum LogLevel log_level, const char* const suffix, Args... args)
   {
      // Bail out if log level bit is not set in the log mask,
      // or the XRootD logging subsystem has not been initialised
      if(!(log_level & XrdSsi::Log.getMsgMask()) ||
         XrdSsi::Log.logger() == nullptr) return;

      std::stringstream logstream;

      logstream << "pid:" << ::getpid() << " tid:" << std::this_thread::get_id() << ' ';

      BuildMessage(logstream, args...);

      XrdSsi::Log.Emsg(suffix, logstream.str().c_str());
   }

   /*!
    * Log contents of a serialized buffer as a string of bytes in hexadecimal format
    */
   static void DumpBuffer(enum LogLevel log_level, const char *buffer, int buflen)
   {
      const int wrap = 40;    // Number of bytes to log on each line of output

      // Bail out if log level bit is not set in the log mask,
      // or the XRootD logging subsystem has not been initialised
      if(!(log_level & XrdSsi::Log.getMsgMask()) ||
         XrdSsi::Log.logger() == nullptr) return;

      std::stringstream logstream;

      logstream << std::hex;

      for(int i = 1; i <= buflen; ++i)
      {
         // Cast to unsigned char so values >127 are not negative, then cast to unsigned int to output as a number not a char...
         logstream << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(*buffer++)) << ' ';

         if(i % wrap == 0 && i != buflen) logstream << std::endl;
      }

      XrdSsi::Log.Say(logstream.str().c_str());
   }

   /*!
    * Dump contents of a protocol buffer to the Logger in JSON format.
    *
    * This version is intended for humans to read, so it uses pretty printing
    */
   static void DumpProtobuf(enum LogLevel log_level, const google::protobuf::Message *message)
   {
      using namespace google::protobuf::util;

      // Bail out if log level bit is not set in the log mask,
      // or the XRootD logging subsystem has not been initialised
      if(!(log_level & XrdSsi::Log.getMsgMask()) ||
         XrdSsi::Log.logger() == nullptr) return;

      std::string logstring;
      JsonPrintOptions options;

      options.add_whitespace = true;
      options.always_print_primitive_fields = true;
      MessageToJsonString(*message, &logstring, options);

      // Delete final endl as the logger adds one as well
      logstring.resize(logstring.size()-1);

      XrdSsi::Log.Say(logstring.c_str());
   }

   /*!
    * Dump contents of a protocol buffer to a string in JSON format.
    *
    * This version is intended for output to stdout, for processing by scripts, so it does not have
    * pretty printing. It can of course be added by post-processing the output, e.g. pipe through :
    *    python -m json.tool
    */
   static std::string DumpProtobuf(const google::protobuf::Message *message)
   {
      using namespace google::protobuf::util;

      std::string logstring;
      JsonPrintOptions options;

      options.always_print_primitive_fields = true;
      MessageToJsonString(*message, &logstring, options);

      return logstring;
   }

private:
   /*!
    * Recursive variadic function to build a log string from an arbitrary number of items of arbitrary type
    */
   template<typename T, typename... Args>
   static void BuildMessage(std::stringstream &logstream, T item, Args... args) 
   {
      logstream << item;
      BuildMessage(logstream, args...);
   }

   /*!
    * Base case function to add one item to the log
    */
   template<typename T>
   static void BuildMessage(std::stringstream &logstream, T item)
   {
      logstream << item;
   }
};

} // namespace XrdSsiPb

