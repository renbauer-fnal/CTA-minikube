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

#pragma once

#include "common/log/LogContext.hpp"
#include "RAOManager.hpp"

namespace castor { namespace tape { namespace tapeserver { namespace rao {

/**
 * Factory of RAOAlgorithmFactory
 * This is where the logic of instanciating the correct RAOAlgorithmFactory is implemented.
 * 
 * This logic is the following:
 * If we were able to query the drive User Data Segments (UDS) limits, then it means
 * that the type of algorithm to instanciate will be an enterprise drive one (instanciate a EnterpriseRAOAlgorithmFactory)
 * 
 * If we were not able to query the drive UDS limits, then it means that we will use a CTA RAO algorithm : linear, random (NonConfigurable RAO Algorithms) or
 * sltf (Configurable RAO Algorithm) 
 * 
 * For implementation, see the implementation of the createAlgorithmFactory() method
 */
class RAOAlgorithmFactoryFactory {
public:
  RAOAlgorithmFactoryFactory(RAOManager & raoManager, cta::log::LogContext & lc);
  /**
   * Returns the correct RAOAlgorithmFactory according to the informations
   * stored in the RAO manager
   */
  std::unique_ptr<RAOAlgorithmFactory> createAlgorithmFactory();
  virtual ~RAOAlgorithmFactoryFactory();
private:
  RAOManager & m_raoManager;
  cta::log::LogContext & m_lc;
};

}}}}
