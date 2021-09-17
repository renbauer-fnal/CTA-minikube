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

#include "common/dataStructures/utils.hpp"

namespace cta {
namespace common {
namespace dataStructures {

std::ostream &operator<<(std::ostream &os, const std::list<TapeFile> &list) {
  os << "(";
  for(auto & af: list) {
    os << af << " ";
  }
  os << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const std::map<uint64_t,std::string> &map) {
  os << "(";
  for(auto it = map.begin(); it != map.end(); it++) {
    os << " key=" << it->first << " value=" << it->second << " ";
  }
  os << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const std::pair<std::string,std::string> &pair){
  os << "(first=" << pair.first << " second=" << pair.second << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const std::map<uint64_t,std::pair<std::string,std::string>> &map) {
  os << "(";
  for(auto it = map.begin(); it != map.end(); it++) {
    os << " key=" << it->first << " value.first=" << it->second.first << "  value.second=" << it->second.second;
  }
  os << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const std::map<std::string,std::pair<uint32_t,TapeFile>> &map) {
  os << "(";
  for(auto it = map.begin(); it != map.end(); it++) {
    os << " key=" << it->first << " value.first=" << it->second.first << "  value.second=" << it->second.second;
  }
  os << ")";
  return os;
}

} // namespace dataStructures
} // namespace common
} // namespace cta
