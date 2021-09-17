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

#include <sqlite3.h>
#include <string>

namespace cta {
namespace rdbms {
namespace wrapper {

/**
 * A helper class for working with SQLite.
 */
class Sqlite {
public:

  /**
   * Returns the string representation of the specified SQLite return code.
   *
   * @param rc The SQLite return code.
   * @return The string representation of the SQLite return code.
   */
  static std::string rcToStr(const int rc);

}; // class SqlLiteStmt

} // namespace wrapper
} // namespace rdbms
} // namespace cta
