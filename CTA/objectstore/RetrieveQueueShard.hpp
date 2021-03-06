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

#include "RetrieveQueue.hpp"

namespace cta { namespace objectstore {
  
class RetrieveQueueShard: public ObjectOps<serializers::RetrieveQueueShard, serializers::RetrieveQueueShard_t>  {
public:
  // Constructor with undefined address
  RetrieveQueueShard(Backend & os);
  
  // Constructor
  RetrieveQueueShard(const std::string & address, Backend & os);
  
  // Upgrader form generic object
  RetrieveQueueShard(GenericObject & go);
  
  // Forbid/hide base initializer
  void initialize() = delete;
  
  // Initializer
  void initialize(const std::string & owner);
  
  // dumper
  std::string dump();
  
  void garbageCollect(const std::string& presumedOwner, AgentReference& agentReference, log::LogContext& lc, cta::catalogue::Catalogue& catalogue) override;

  struct JobInfo {
    uint64_t size;
    std::string address;
    uint32_t copyNb;
    uint64_t priority;
    uint64_t minRetrieveRequestAge;
    time_t startTime;
    uint64_t fSeq;
    std::string mountPolicyName;
    optional<RetrieveQueue::JobDump::ActivityDescription> activityDescription;
    optional<std::string> diskSystemName;
  };
  std::list<JobInfo> dumpJobs();
  
  /** Variant function allowing shard to shard job transfer (not needed) archives, 
   * which do not split. */
  std::list<RetrieveQueue::JobToAdd> dumpJobsToAdd();
  
  struct JobsSummary {
    uint64_t jobs;
    uint64_t bytes;
    uint64_t minFseq;
    uint64_t maxFseq;
  };
  JobsSummary getJobsSummary();
  
  struct RetrieveQueueJobsToAddLess {
    constexpr bool operator() (const RetrieveQueue::JobToAdd& lhs, 
      const RetrieveQueue::JobToAdd& rhs) { return lhs.fSeq < rhs.fSeq; }
  };
  
  typedef std::multiset<RetrieveQueue::JobToAdd, RetrieveQueueJobsToAddLess> JobsToAddSet;
  
  struct RetrieveQueueSerializedJobsToAddLess {
    bool operator() (const serializers::RetrieveJobPointer& lhs, 
      const serializers::RetrieveJobPointer& rhs) { return lhs.fseq() < rhs.fseq(); }
  };
  typedef std::multiset<serializers::RetrieveJobPointer, 
    RetrieveQueueSerializedJobsToAddLess> SerializedJobsToAddSet;
private:
  /**
   * adds job
   */
  void addJob(const RetrieveQueue::JobToAdd & jobToAdd);
  
  /**
   * adds batch of jobs.
   */
  void addJobsInPlace(JobsToAddSet & jobsToAdd);
  
  void addJobsThroughCopy(JobsToAddSet & jobsToAdd);
  
public:
  void addJobsBatch(JobsToAddSet & jobToAdd);
  
  
  struct RemovalResult {
    uint64_t jobsRemoved = 0;
    uint64_t jobsAfter = 0;
    uint64_t bytesRemoved = 0;
    uint64_t bytesAfter = 0;
    std::list<JobInfo> removedJobs;
  };
  /**
   * Removes jobs from shard (and from the to remove list). Returns list of removed jobs.
   */
  RemovalResult removeJobs(const std::list<std::string> & jobsToRemove);
  
  RetrieveQueue::CandidateJobList getCandidateJobList(uint64_t maxBytes, uint64_t maxFiles,
    const std::set<std::string> & retrieveRequestsToSkip, const std::set<std::string> & diskSystemsToSkip);
  
  /** Re compute summaries in case they do not match the array content. */
  void rebuild();
  
};

}} // namespace cta::objectstore