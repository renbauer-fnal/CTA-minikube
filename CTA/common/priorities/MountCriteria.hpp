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

#include <stdint.h>

namespace cta {

/**
 * Class representing the criteria be met in order to justify mounting a tape.
 */
class MountCriteria {
public:

  /**
   * Constructor.
   */
  MountCriteria();

  /**
   * Constructor.
   *
   * @param nbBytes The minimum number of queued bytes required to justify a
   * mount.
   * @param nbFiles The minimum number of queued files required to justify a
   * mount.
   * @param ageInSecs The minimum age in seconds of queued data required to
   * justify a mount.
   */
  MountCriteria(const uint64_t nbBytes, const uint64_t nbFiles,
    const uint64_t ageInSecs);

  /**
   * Returns the minimum number of queued bytes required to justify a mount.
   *
   * @return The minimum number of queued bytes required to justify a mount.
   */
  uint64_t getNbBytes() const throw();

  /**
   * Returns the minimum number of queued files required to justify a mount.
   *
   * @return The minimum number of queued files required to justify a mount.
   */
  uint64_t getNbFiles() const throw();

  /**
   * Returns the minimum age in seconds of queued data required to justify a
   * mount.
   *
   * @return The minimum age in seconds of queued data required to justify a
   * mount.
   */
  uint64_t getAgeInSecs() const throw();

private:

  /**
   * The minimum number of queued bytes required to justify a mount.
   */
  uint64_t m_nbBytes;

  /**
   * The minimum number of queued files required to justify a mount.
   */
  uint64_t m_nbFiles;

  /**
   * The minimum age in seconds of queued data required to justify a mount.
   */
  uint64_t m_ageInSecs;

}; // class MountCriteria

} // namespace cta
