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

#include "objectstore/Agent.hpp"
#include "objectstore/AgentReference.hpp"
#include "objectstore/Backend.hpp"

namespace cta { namespace objectstore { 

class BackendPopulator {
  
public:
  /**
   * Constructor
   * 
   * @param be The objectstore backend
   */
  BackendPopulator(cta::objectstore::Backend & be, const std::string &agentType, const cta::log::LogContext & lc);
  
  /**
   * Destructor
   */
  virtual ~BackendPopulator() throw();

  /**
   * Returns the agent
   * 
   * @return the agent
   */
  cta::objectstore::AgentReference & getAgentReference();
  
  /**
   * allow leaving bahind non-empty agents.
   */
  void leaveNonEmptyAgentsBehind();
  
private:
  /**
   * The objectstore backend
   */
  cta::objectstore::Backend & m_backend;
  
  /**
   * The agent
   */
  cta::objectstore::AgentReference m_agentReference;
  
  /**
   * A log context (copied) in order to be able to log in destructor.
   */
  cta::log::LogContext m_lc;
  
  /**
   * A switch allowing leaving behind a non-empty agent for garbage collection pickup.
   */
  bool m_leaveNonEmptyAgentBehind = false;
};

}}