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

#include "JsonStatisticsService.hpp"

namespace cta {
namespace statistics {
  
JsonStatisticsService::JsonStatisticsService(OutputStream * output):m_output(output),m_input(nullptr) {

}

JsonStatisticsService::JsonStatisticsService(OutputStream * output, InputStream * input):m_output(output),m_input(input) {

}

void JsonStatisticsService::saveStatistics(const cta::statistics::Statistics& statistics){
  *m_output << statistics;
} 

std::unique_ptr<cta::statistics::Statistics> JsonStatisticsService::getStatistics(){
  throw cta::exception::Exception("In JsonStatisticsService::getStatistics(), method not implemented.");
} 

void JsonStatisticsService::updateStatisticsPerTape(){
  throw cta::exception::Exception("In JsonStatistics::updateStatisticsPerTape(), method not implemented.");
} 


JsonStatisticsService::~JsonStatisticsService() {
}

}}