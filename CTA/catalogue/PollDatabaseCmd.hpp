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

#include "catalogue/CmdLineTool.hpp"

namespace cta {
namespace catalogue {

/**
 * Command-line tool that simply polls the database with a simple SQL SELECT
 * statement.
 *
 * The purpose of this tool is to faciliate the development and testing of the
 * reconnect logic of CTA database connections.  The database server is
 * configured to break some connections.  If the reconnect logic is working
 * correctly then the polling loop of this command should trigger automatic
 * and successfull reconnects.
 */
class PollDatabaseCmd: public CmdLineTool {
public:

  /**
   * Constructor.
   *
   * @param inStream Standard input stream.
   * @param outStream Standard output stream.
   * @param errStream Standard error stream.
   */
  PollDatabaseCmd(std::istream &inStream, std::ostream &outStream, std::ostream &errStream);

  /**
   * Destructor.
   */
  ~PollDatabaseCmd() noexcept;

private:

  /**
   * An exception throwing version of main().
   *
   * @param argc The number of command-line arguments including the program name.
   * @param argv The command-line arguments.
   * @return The exit value of the program.
   */
  int exceptionThrowingMain(const int argc, char *const *const argv) override;

  /**
   * Prints the usage message of the command-line tool.
   *
   * @param os The output stream to which the usage message is to be printed.
   */
  void printUsage(std::ostream &os) override;

}; // class PollDatabaseCmd

} // namespace catalogue
} // namespace cta
