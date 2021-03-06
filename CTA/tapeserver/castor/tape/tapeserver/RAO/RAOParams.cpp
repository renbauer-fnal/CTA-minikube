/*
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2003-2021 CERN
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

#include "RAOParams.hpp"
#include "common/exception/Exception.hpp"
#include "common/utils/utils.hpp"

namespace castor { namespace tape { namespace tapeserver { namespace rao {
  
const std::map<std::string,RAOParams::RAOAlgorithmType> RAOParams::c_raoAlgoStringTypeMap = {
  {"linear",RAOParams::RAOAlgorithmType::linear},
  {"random",RAOParams::RAOAlgorithmType::random},
  {"sltf",RAOParams::RAOAlgorithmType::sltf}
};  

RAOParams::RAOParams():m_useRAO(false){}

RAOParams::RAOParams(const bool useRAO, const std::string& raoAlgorithmName, const std::string & raoAlgorithmOptions, const std::string & vid):m_useRAO(useRAO), m_raoAlgorithmName(raoAlgorithmName), 
    m_raoAlgorithmOptions(raoAlgorithmOptions), m_vid(vid) {

}

RAOParams::RAOParams(const RAOParams& other) {
  if(this != &other){
    m_useRAO = other.m_useRAO;
    m_raoAlgorithmName = other.m_raoAlgorithmName;
    m_raoAlgorithmOptions = other.m_raoAlgorithmOptions;
    m_vid = other.m_vid;
  }
}

RAOParams& RAOParams::operator=(const RAOParams& other) {
  if(this != &other){
    m_useRAO = other.m_useRAO;
    m_raoAlgorithmName = other.m_raoAlgorithmName;
    m_raoAlgorithmOptions = other.m_raoAlgorithmOptions;
    m_vid = other.m_vid;
  }
  return *this;
}

bool RAOParams::useRAO() const {
  return m_useRAO;
}

std::string RAOParams::getRAOAlgorithmName() const {
  return m_raoAlgorithmName;
}

RAOOptions RAOParams::getRAOAlgorithmOptions() const {
  return m_raoAlgorithmOptions;
}

void RAOParams::disableRAO(){
  m_useRAO = false;
}

RAOParams::RAOAlgorithmType RAOParams::getAlgorithmType() const {
  try {
    return c_raoAlgoStringTypeMap.at(m_raoAlgorithmName);  
  } catch (const std::out_of_range &){
    throw cta::exception::Exception("The algorithm name provided by the RAO configuration does not match any RAO algorithm type.");
  }
}

std::string RAOParams::getCTARAOAlgorithmNameAvailable() const {
  std::string ret;
  for(auto & kv: c_raoAlgoStringTypeMap){
    ret += kv.first + " ";
  }
  if(ret.size()){
    //remove last space
    ret.resize(ret.size()-1);
  }
  return ret;
}

std::string RAOParams::getMountedVid() const {
  if(m_vid.empty()){
    throw cta::exception::Exception("In RAOData::getMountedVid(), no mounted vid found.");
  }
  return m_vid;
}





}}}}