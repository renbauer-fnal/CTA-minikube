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

#include <string>

namespace cta {

/**
 * The path of a file in a remote storage system of the form
 * "scheme://path".
 */
class RemotePath {
public:

  /**
   * Constructor.
   */
  RemotePath();
  
  /**
   * Setter
   * @param raw
   */
  void setPath(const std::string &raw);
  
  /**
   * Constructor.
   *
   * @param raw The raw path in the form "scheme:after_scheme".
   */
  RemotePath(const std::string &raw);

  /**
   * Equals operator.
   */
  bool operator==(const RemotePath &rhs) const;
  
  /**
   * Less than operator
   */
  bool operator<(const RemotePath &rhs) const;

  /**
   * Returns true if the remote path is empty.
   */
  bool empty() const;

  /**
   * Returns the raw path in the form "scheme:after_schem".
   */
  const std::string &getRaw() const;

  /**
   * Returns the scheme part of the remote path.
   */
  const std::string &getScheme() const;

  /**
   * Returns the part of the remote path after the scheme as in the raw path
   * "scheme:after_scheme" would result in "after_scheme" being returned.
   *
   * @return The part of the remote path after the scheme as in the raw path
   * "scheme:after_scheme" would result in "after_scheme" being returned.
   */
  const std::string &getAfterScheme() const;

private:

  /**
   * The raw path in the form "scheme:after_scheme".
   */
  std::string m_raw;

  /**
   * The scheme part of the remote path.
   */
  std::string m_scheme;

  /**
   * The part of the remote path after the scheme.  For example the part after
   * the scheme for the raw path "scheme:after_scheme" would be "after_scheme".
   */
  std::string m_afterScheme;

}; // class RemotePath

} // namespace cta
