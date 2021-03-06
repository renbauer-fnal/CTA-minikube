/*
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2015-2021 CERN
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

#include "common/log/Logger.hpp"
#include "common/threading/Mutex.hpp"

namespace cta {
namespace log {

/**
 * Class implementaing the API of the CASTOR logging system.
 */
class FileLogger: public Logger {
public:

  /**
   * Constructor
   *
   * @param hostName The host name to be prepended to every log message.
   * @param programName The name of the program to be prepended to every log
   * message.
   * @param filePath path to the log file.
   * @param logMask The log mask.
   */
  FileLogger(const std::string &hostName, const std::string &programName, const std::string &filePath,
    const int logMask);

  /**
   * Destructor.
   */
  ~FileLogger();

  /**
   * Prepares the logger object for a call to fork().
   *
   * No further calls to operator() should be made after calling this
   * method until the call to fork() has completed.
   */
  void prepareForFork() ;

protected:

  /**
   * Mutex used to protect the critical section of the StringLogger
   * object.
   */
  threading::Mutex m_mutex;

  /**
   * The output file handle.
   */
  int m_fd=-1;

  /**
   * Writes the specified msg to the underlying logging system.
   *
   * This method is to be implemented by concrete sub-classes of the Logger
   * class.
   *
   * Please note it is the responsibility of a concrete sub-class to decide
   * whether or not to use the specified log message header.  For example, the
   * SysLogLogger sub-class does not use the header.  Instead it relies on
   * rsyslog to provide a header.
   *
   * @param header The header of the message to be logged.  It is the
   * esponsibility of the concrete sub-class
   * @param body The body of the message to be logged.
   */
  void writeMsgToUnderlyingLoggingSystem(const std::string &header, const std::string &body) override;

}; // class StringLogger

} // namespace log
} // namespace cta
