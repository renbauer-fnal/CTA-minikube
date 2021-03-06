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

#include "catalogue/TapeForWriting.hpp"

namespace cta {
namespace catalogue {

//------------------------------------------------------------------------------
// constructor
//------------------------------------------------------------------------------
TapeForWriting::TapeForWriting():
  lastFSeq(0),
  capacityInBytes(0),
  dataOnTapeInBytes(0) {
}

//------------------------------------------------------------------------------
// operator==
//------------------------------------------------------------------------------
bool TapeForWriting::operator==(const TapeForWriting &rhs) const {
  return vid == rhs.vid;
}

//------------------------------------------------------------------------------
// operator<<
//------------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const TapeForWriting &obj) {
  os <<
    "{"                  <<
    "vid="               << obj.vid << "," <<
    "lastFseq="          << obj.lastFSeq << "," <<
    "capacityInBytes="   << obj.capacityInBytes << "," <<
    "dataOnTapeInBytes=" << obj.dataOnTapeInBytes <<
    "}";

  return os;
}

} // namespace catalogue
} // namespace cta
