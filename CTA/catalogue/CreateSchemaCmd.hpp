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
#include "rdbms/Conn.hpp"

namespace cta {
namespace catalogue {

/**
 * Command-line tool for creating the catalogue schema.
 */
class CreateSchemaCmd: public CmdLineTool {
public:

  /**
   * Constructor.
   *
   * @param inStream Standard input stream.
   * @param outStream Standard output stream.
   * @param errStream Standard error stream.
   */
  CreateSchemaCmd(std::istream &inStream, std::ostream &outStream, std::ostream &errStream);

  /**
   * Destructor.
   */
  ~CreateSchemaCmd() noexcept;

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

  /**
   * Returns true if the table with the specified name exists in the database
   * schema of teh specified database connection.
   *
   * @param tableName The name of the database table.
   * @param conn The database connection.
   * @return True if the table exists.
   */
  bool tableExists(const std::string tableName, rdbms::Conn &conn) const;

  /**
   * Parses the specified string of multiple SQL statements separated by
   * semicolons and calls executeNonQuery() for each statement found.
   *
   * Please note that this method does not support statements that themselves
   * contain one more semicolons.
   *
   * @param conn The database connection.
   * @param sqlStmts Multiple SQL statements separated by semicolons.
   * Statements that themselves contain one more semicolons are not supported.
   */
  void executeNonQueries(rdbms::Conn &conn, const std::string &sqlStmts);

}; // class CreateSchemaCmd

} // namespace catalogue
} // namespace cta
