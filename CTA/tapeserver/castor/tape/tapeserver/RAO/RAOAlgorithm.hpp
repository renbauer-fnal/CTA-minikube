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

#include <vector>
#include <memory>
#include "scheduler/RetrieveJob.hpp"
#include "common/log/TimingList.hpp"

namespace castor { namespace tape { namespace tapeserver { namespace rao {
  
/**
 * Abstract class that represents an RAO algorithm
 */
class RAOAlgorithm {
protected:
  //Timing list to store the timings of the different steps of the RAO algorithm
  cta::log::TimingList m_raoTimings;
public:
  virtual ~RAOAlgorithm();
  
  /**
   * Returns the vector of indexes of the jobs passed in parameter
   * sorted according to an algorithm
   * @param jobs the jobs to perform RAO on
   * @return the vector of indexes sorted by an algorithm applied on the jobs passed in parameter
   */
  virtual std::vector<uint64_t> performRAO(const std::vector<std::unique_ptr<cta::RetrieveJob>> & jobs) = 0;
  
  /**
   * Returns the timings the RAO Algorithm took to perform each step
   * @return the timings the RAO Algorithm took to perform each step
   */
  cta::log::TimingList getRAOTimings();
  
  /**
   * Returns the name of the RAO algorithm
   * @return the name of the RAO algorithm
   */
  virtual std::string getName() const = 0;
};

}}}}