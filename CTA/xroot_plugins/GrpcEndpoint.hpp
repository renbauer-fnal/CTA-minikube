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

#include <xroot_plugins/Namespace.hpp>
#include <xroot_plugins/GrpcClient.hpp>

namespace cta { namespace grpc { 

class Endpoint
{
public:
  Endpoint(const Namespace &endpoint) :
    m_grpcClient(::eos::client::GrpcClient::Create(endpoint.endpoint, endpoint.token)) { }

  std::string getPath(const std::string &diskFileId) const;

private:
  std::unique_ptr<::eos::client::GrpcClient> m_grpcClient;
};


class EndpointMap
{
public:
  EndpointMap(NamespaceMap_t nsMap) {
    for(auto &ns : nsMap) {
      m_endpointMap.insert(std::make_pair(ns.first, Endpoint(ns.second)));
    }
  }

  std::string getPath(const std::string &diskInstance, const std::string &diskFileId) const {
    auto ep_it = m_endpointMap.find(diskInstance);
    if(ep_it == m_endpointMap.end()) {
      return "Namespace for disk instance \"" + diskInstance + "\" is not configured in the CTA Frontend";
    } else {
      return ep_it->second.getPath(diskFileId);
    }
  }

private:
  std::map<std::string, Endpoint> m_endpointMap;
};

}} // namespace cta::grpc
