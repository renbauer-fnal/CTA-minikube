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

#include "catalogue/ArchiveFileRow.hpp"

namespace cta {
namespace catalogue {

//------------------------------------------------------------------------------
// constructor
//------------------------------------------------------------------------------
ArchiveFileRow::ArchiveFileRow() :
  archiveFileId(0),
  size(0),
  creationTime(0),
  reconciliationTime(0) {
}

//------------------------------------------------------------------------------
// operator==
//------------------------------------------------------------------------------
bool ArchiveFileRow::operator==(const ArchiveFileRow &rhs) const {
  return
    archiveFileId == rhs.archiveFileId &&
    diskInstance == rhs.diskInstance &&
    diskFileId == rhs.diskFileId &&
    diskFileOwnerUid == rhs.diskFileOwnerUid &&
    diskFileGid == rhs.diskFileGid &&
    size == rhs.size &&
    checksumBlob == rhs.checksumBlob &&
    storageClassName == rhs.storageClassName;
}

//------------------------------------------------------------------------------
// operator<<
//------------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const ArchiveFileRow &obj) {
  os <<
  "{"
  "archiveFileId=" << obj.archiveFileId <<
  "diskInstance=" << obj.diskInstance <<
  "diskFileId=" << obj.diskFileId <<
  "diskFileOwnerUid=" << obj.diskFileOwnerUid <<
  "diskFileGid=" << obj.diskFileGid <<
  "size=" << obj.size <<
  "checksumBlob=" << obj.checksumBlob <<
  "storageClassName=" << obj.storageClassName <<
  "}";
  return os;
}

} // namespace catalogue
} // namespace cta
