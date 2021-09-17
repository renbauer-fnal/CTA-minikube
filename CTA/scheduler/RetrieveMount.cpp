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

#include "scheduler/RetrieveMount.hpp"
#include "common/Timer.hpp"
#include "common/log/TimingList.hpp"
#include "objectstore/Backend.hpp"
#include "disk/DiskSystem.hpp"

//------------------------------------------------------------------------------
// constructor
//------------------------------------------------------------------------------
cta::RetrieveMount::RetrieveMount(cta::catalogue::Catalogue &catalogue): m_sessionRunning(false), m_catalogue(catalogue) {
}

//------------------------------------------------------------------------------
// constructor
//------------------------------------------------------------------------------
cta::RetrieveMount::RetrieveMount(
  cta::catalogue::Catalogue &catalogue,
  std::unique_ptr<SchedulerDatabase::RetrieveMount> dbMount): 
  m_sessionRunning(false), m_catalogue(catalogue) {
  m_dbMount.reset(dbMount.release());
}

//------------------------------------------------------------------------------
// getMountType()
//------------------------------------------------------------------------------
cta::common::dataStructures::MountType cta::RetrieveMount::getMountType() const{
  return cta::common::dataStructures::MountType::Retrieve;
}

//------------------------------------------------------------------------------
// getNbFiles()
//------------------------------------------------------------------------------
uint32_t cta::RetrieveMount::getNbFiles() const {
  return m_dbMount->nbFilesCurrentlyOnTape;
}

//------------------------------------------------------------------------------
// getVid()
//------------------------------------------------------------------------------
std::string cta::RetrieveMount::getVid() const{
  return m_dbMount->mountInfo.vid;
}

//------------------------------------------------------------------------------
// getActivity()
//------------------------------------------------------------------------------
cta::optional<std::string> cta::RetrieveMount::getActivity() const {
  return m_dbMount->mountInfo.activity;
}

//------------------------------------------------------------------------------
// getMountTransactionId()
//------------------------------------------------------------------------------
std::string cta::RetrieveMount::getMountTransactionId() const{
  std::stringstream id;
  if (!m_dbMount.get())
    throw exception::Exception("In cta::RetrieveMount::getMountTransactionId(): got NULL dbMount");
  id << m_dbMount->mountInfo.mountId;
  return id.str();
}

//------------------------------------------------------------------------------
// getMountTapePool()
//------------------------------------------------------------------------------
std::string cta::RetrieveMount::getPoolName() const{
  std::stringstream sTapePool;
  if (!m_dbMount.get())
    throw exception::Exception("In cta::RetrieveMount::getPoolName(): got NULL dbMount");
  sTapePool << m_dbMount->mountInfo.tapePool;
  return sTapePool.str();
}

//------------------------------------------------------------------------------
// getVo()
//------------------------------------------------------------------------------
std::string cta::RetrieveMount::getVo() const
{
    std::stringstream sVo;
    if(!m_dbMount.get())
        throw exception::Exception("In cta::RetrieveMount::getVo(): got NULL dbMount");
    sVo<<m_dbMount->mountInfo.vo;
    return sVo.str();
}

//------------------------------------------------------------------------------
// getMediaType()
//------------------------------------------------------------------------------
std::string cta::RetrieveMount::getMediaType() const
{
    std::stringstream sMediaType;
    if(!m_dbMount.get())
        throw exception::Exception("In cta::RetrieveMount::getMediaType(): got NULL dbMount");
    sMediaType<<m_dbMount->mountInfo.mediaType;
    return sMediaType.str();
}

//------------------------------------------------------------------------------
// getVendor()
//------------------------------------------------------------------------------
std::string cta::RetrieveMount::getVendor() const
{
    std::stringstream sVendor;
    if(!m_dbMount.get())
        throw exception::Exception("In cta::RetrieveMount::getVendor(): got NULL dbMount");
    sVendor<<m_dbMount->mountInfo.vendor;
    return sVendor.str();
}

//------------------------------------------------------------------------------
// getCapacityInBytes()
//------------------------------------------------------------------------------
uint64_t cta::RetrieveMount::getCapacityInBytes() const {
    if(!m_dbMount.get())
        throw exception::Exception("In cta::RetrieveMount::getVendor(): got NULL dbMount");
    return m_dbMount->mountInfo.capacityInBytes;
}

//------------------------------------------------------------------------------
// getNextJobBatch()
//------------------------------------------------------------------------------
std::list<std::unique_ptr<cta::RetrieveJob> > cta::RetrieveMount::getNextJobBatch(uint64_t filesRequested, uint64_t bytesRequested,
    log::LogContext& logContext) {
  if (!m_sessionRunning)
    throw SessionNotRunning("In RetrieveMount::getNextJobBatch(): trying to get job from complete/not started session");
  // Get the current file systems list from the catalogue
  disk::DiskSystemList diskSystemList;
  diskSystemList = m_catalogue.getAllDiskSystems();
  diskSystemList.setFetchEosFreeSpaceScript(m_fetchEosFreeSpaceScript);
  // TODO: the diskSystemFreeSpaceList could be made a member of the retrieve mount and cache the fetched values, limiting the re-querying
  // of the disk systems free space.
  disk::DiskSystemFreeSpaceList diskSystemFreeSpaceList (diskSystemList);
  // Try and get a new job from the DB. The DB mount (in memory object) is taking care of reserving the free space for the popped 
  // elements and query the disk systems, via the diskSystemFreeSpaceList object.
  auto dbJobBatch = m_dbMount->getNextJobBatch(filesRequested, bytesRequested, diskSystemFreeSpaceList, logContext);
  std::list<std::unique_ptr<RetrieveJob>> ret;
  // We prepare the response
  for (auto & sdrj: dbJobBatch) {
    ret.emplace_back(new RetrieveJob(this,
      sdrj->retrieveRequest, sdrj->archiveFile, sdrj->selectedCopyNb,
      PositioningMethod::ByBlock));
    ret.back()->m_dbJob.reset(sdrj.release());
  }
  return ret;
}

//------------------------------------------------------------------------------
// waitAndFinishSettingJobsBatchRetrieved()
//------------------------------------------------------------------------------
void cta::RetrieveMount::flushAsyncSuccessReports(std::queue<std::unique_ptr<cta::RetrieveJob> >& successfulRetrieveJobs, cta::log::LogContext& logContext) {
  std::list<std::unique_ptr<cta::RetrieveJob> > validatedSuccessfulRetrieveJobs; //List to ensure the destruction of the retrieve jobs at the end of this method
  std::list<cta::SchedulerDatabase::RetrieveJob *> validatedSuccessfulDBRetrieveJobs;
  std::unique_ptr<cta::RetrieveJob> job;
  double waitUpdateCompletionTime=0;
  double jobBatchFinishingTime=0;
  double schedulerDbTime=0;
  uint64_t files=0;
  uint64_t bytes=0;
  utils::Timer t;
  log::TimingList tl;
  try {
    while (!successfulRetrieveJobs.empty()) {
      job = std::move(successfulRetrieveJobs.front());
      successfulRetrieveJobs.pop();
      if (!job.get()) continue;
      files++;
      bytes+=job->archiveFile.fileSize;
      validatedSuccessfulDBRetrieveJobs.emplace_back(job->m_dbJob.get());
      validatedSuccessfulRetrieveJobs.emplace_back(std::move(job));
      job.reset();
    }
    waitUpdateCompletionTime=t.secs(utils::Timer::resetCounter);
    tl.insertAndReset("waitUpdateCompletionTime",t);
    // Complete the cleaning up of the jobs in the mount
    m_dbMount->flushAsyncSuccessReports(validatedSuccessfulDBRetrieveJobs, logContext);
    jobBatchFinishingTime=t.secs();
    tl.insertOrIncrement("jobBatchFinishingTime",jobBatchFinishingTime);
    schedulerDbTime=jobBatchFinishingTime + waitUpdateCompletionTime;
    tl.insertOrIncrement("schedulerDbTime",schedulerDbTime);
    {
      cta::log::ScopedParamContainer params(logContext);
      params.add("successfulRetrieveJobs", successfulRetrieveJobs.size())
            .add("files", files)
            .add("bytes", bytes);
      tl.addToLog(params);
      //TODO : if repack, add log to say that the jobs were marked as RJS_Succeeded
      logContext.log(cta::log::DEBUG,"In cta::RetrieveMount::flushAsyncSuccessReports(): deleted complete retrieve jobs.");
    }
  } catch(const cta::objectstore::Backend::NoSuchObject &ex){
    cta::log::ScopedParamContainer params(logContext);
    params.add("exceptionMessageValue", ex.getMessageValue());
    if (job.get()) {
      params.add("fileId", job->archiveFile.archiveFileID)
            .add("diskInstance", job->archiveFile.diskInstance)
            .add("diskFileId", job->archiveFile.diskFileId)
            .add("lastKnownDiskPath", job->archiveFile.diskFileInfo.path);
    }
    const std::string msg_error="In cta::RetrieveMount::flushAsyncSuccessReports(): unable to access jobs, they do not exist in the objectstore.";
    logContext.log(cta::log::WARNING, msg_error);
  } catch(const cta::exception::Exception& e){
    cta::log::ScopedParamContainer params(logContext);
    params.add("exceptionMessageValue", e.getMessageValue());
    if (job.get()) {
      params.add("fileId", job->archiveFile.archiveFileID)
            .add("diskInstance", job->archiveFile.diskInstance)
            .add("diskFileId", job->archiveFile.diskFileId)
            .add("lastKnownDiskPath", job->archiveFile.diskFileInfo.path);
    }
    const std::string msg_error="In cta::RetrieveMount::flushAsyncSuccessReports(): got an exception";
    logContext.log(cta::log::ERR, msg_error);
    logContext.logBacktrace(cta::log::ERR, e.backtrace());
    // Failing here does not really affect the session so we can carry on. Reported jobs are reported, non-reported ones
    // will be retried.
  } catch(const std::exception& e){
    cta::log::ScopedParamContainer params(logContext);
    params.add("exceptionWhat", e.what());
    if (job.get()) {
      params.add("fileId", job->archiveFile.archiveFileID)
            .add("diskInstance", job->archiveFile.diskInstance)
            .add("diskFileId", job->archiveFile.diskFileId)
            .add("lastKnownDiskPath", job->archiveFile.diskFileInfo.path);
    }
    const std::string msg_error="In cta::RetrieveMount::flushAsyncSuccessReports(): got an standard exception";
    logContext.log(cta::log::ERR, msg_error);
    // Failing here does not really affect the session so we can carry on. Reported jobs are reported, non-reported ones
    // will be retried.
  }
}

//------------------------------------------------------------------------------
// createDiskReporter()
//------------------------------------------------------------------------------
cta::disk::DiskReporter* cta::RetrieveMount::createDiskReporter(std::string& URL) {
  return m_reporterFactory.createDiskReporter(URL);
}

//------------------------------------------------------------------------------
// tapeComplete()
//------------------------------------------------------------------------------
void cta::RetrieveMount::tapeComplete() {
  m_tapeRunning = false;
  if (!m_diskRunning) {
    // Just set the session as complete in the DB.
    m_dbMount->complete(time(NULL));
    // and record we are done with the mount
    m_sessionRunning = false;
  } else {
    // This is a special case: we have to report the tape server is draining
    // its memory to disk
    setDriveStatus(cta::common::dataStructures::DriveStatus::DrainingToDisk);
  }
}

//------------------------------------------------------------------------------
// diskComplete()
//------------------------------------------------------------------------------
void cta::RetrieveMount::diskComplete() {
  m_diskRunning = false;
  if (!m_tapeRunning) {
    // Just set the session as complete in the DB.
    cta::SchedulerDatabase::RetrieveMount  * ptr = m_dbMount.get();
    ptr=ptr;
    m_dbMount->complete(time(NULL));
    // and record we are done with the mount
    m_sessionRunning = false;
  }
}

//------------------------------------------------------------------------------
// abort()
//------------------------------------------------------------------------------
void cta::RetrieveMount::abort() {
  diskComplete();
  tapeComplete();
}

//------------------------------------------------------------------------------
// setDriveStatus()
//------------------------------------------------------------------------------
void cta::RetrieveMount::setDriveStatus(cta::common::dataStructures::DriveStatus status, const cta::optional<std::string> & reason) {
  m_dbMount->setDriveStatus(status, time(NULL), reason);
}

//------------------------------------------------------------------------------
// setTapeSessionStats()
//------------------------------------------------------------------------------
void cta::RetrieveMount::setTapeSessionStats(const castor::tape::tapeserver::daemon::TapeSessionStats &stats) {
  m_dbMount->setTapeSessionStats(stats);
}

//------------------------------------------------------------------------------
// setTapeMounted()
//------------------------------------------------------------------------------
void cta::RetrieveMount::setTapeMounted(cta::log::LogContext& logContext) const {
  utils::Timer t;    
  log::ScopedParamContainer spc(logContext);
  try {
    m_catalogue.tapeMountedForRetrieve(m_dbMount->getMountInfo().vid, m_dbMount->getMountInfo().drive);
    auto catalogueTime = t.secs(cta::utils::Timer::resetCounter);
    spc.add("catalogueTime", catalogueTime);
    logContext.log(log::INFO, "In RetrieveMount::setTapeMounted(): success.");
  } catch (cta::exception::Exception &ex) {
    auto catalogueTimeFailed = t.secs(cta::utils::Timer::resetCounter);
    spc.add("catalogueTime", catalogueTimeFailed);
    logContext.log(cta::log::WARNING,
      "Failed to update catalogue for the tape mounted for retrieve.");
  }    
}

//------------------------------------------------------------------------------
// bothSidesComplete()
//------------------------------------------------------------------------------
bool cta::RetrieveMount::bothSidesComplete() {
  return !(m_diskRunning || m_tapeRunning);
}

//------------------------------------------------------------------------------
// setEosFreeSpaceScriptName()
//------------------------------------------------------------------------------
void cta::RetrieveMount::setFetchEosFreeSpaceScript(const std::string& name){
  m_fetchEosFreeSpaceScript = name;
}

//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
cta::RetrieveMount::~RetrieveMount() throw() {
}
