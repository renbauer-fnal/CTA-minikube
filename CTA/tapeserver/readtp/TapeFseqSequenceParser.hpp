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

#include "common/exception/Exception.hpp"
#include "tapeserver/readtp/TapeFseqRangeListSequence.hpp"

#include <list>

namespace cta {
namespace tapeserver {
namespace readtp {

/**
 * Helper class to parse tape file sequence parameter strings.
 */
class TapeFileSequenceParser {
public:

  /**
   * Parse the specified tape file sequence parameter string and store the
   * resulting ranges into m_parsedCommandLine.tapeFseqRanges.
   *
   * The syntax rules for a tape file sequence specification are:
   * <ul>
   *  <li>  f1            File f1.
   *  <li>  f1-f2         Files f1 to f2 included.
   *  <li>  f1-           Files from f1 to the last file of the tape.
   *  <li>  f1-f2,f4,f6-  Series of ranges "," separated.
   * </ul>
   *
   * @param str The string received as an argument for the TapeFileSequence
   * option.
   * @return The resulting list of tape file sequence ranges.
   */
  static TapeFseqRangeList parse(char *const str)
    ;

}; // class TapeFileSequenceParser

} // namespace readtp
} // namespace tapeserver
} // namespace cta