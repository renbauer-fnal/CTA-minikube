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

#include "common/exception/Exception.hpp"
#include "scheduler/RetrieveJob.hpp"
#include "scheduler/RetrieveMount.hpp"
#include "scheduler/SchedulerDatabase.hpp"
#include "scheduler/TapeMount.hpp"
#include "disk/DiskReporterFactory.hpp"
#include "catalogue/Catalogue.hpp"

#include <memory>
#include <queue>

namespace cta {
  
  /**
   * The class driving a retrieve mount.
   * The class only has private constructors as it is instanciated by
   * the Scheduler class.
   */
  class RetrieveMount: public TapeMount {
    friend class Scheduler;
  protected:    
    /**
     * Constructor.
     * @param catalogue The file catalogue interface.
     */
    RetrieveMount(cta::catalogue::Catalogue &catalogue);
    
    /**
     * Constructor.
     * @param catalogue The file catalogue interface.
     * @param dbMount The database representation of this mount.
     */
    RetrieveMount(cta::catalogue::Catalogue &catalogue, std::unique_ptr<cta::SchedulerDatabase::RetrieveMount> dbMount);

  public:

    CTA_GENERATE_EXCEPTION_CLASS(WrongMountType);
    CTA_GENERATE_EXCEPTION_CLASS(NotImplemented);

    /**
     * Returns The type of this tape mount.
     *
     * @return The type of this tape mount.
     */
    virtual cta::common::dataStructures::MountType getMountType() const;

    /**
     * Returns the volume identifier of the tape to be mounted.
     *
     * @return The volume identifier of the tape to be mounted.
     */
    virtual std::string getVid() const;
    
    /**
     * Returns the (optional) activity for this mount.
     * 
     * @return 
     */
    optional<std::string> getActivity() const override;

    
    /**
     * Returns the mount transaction id.
     *
     * @return The mount transaction id.
     */
    virtual std::string getMountTransactionId() const;
    
    /**
     * Returns the mount transaction id.
     *
     * @return The mount transaction id.
     */
    uint32_t getNbFiles() const override;
    
     /**
     * Returns the tape pool of the tape to be mounted
     * @return The tape pool of the tape to be mounted
     */
    std::string getPoolName() const;
    
    /**
     * Returns the virtual organization in which the tape
     * belongs
     * @return the vo in which the tape belongs
     */
    std::string getVo() const;
    
    /**
     * Returns the media type of the tape
     * @return de media type of the tape
     */
    std::string getMediaType() const;
    
    /**
     * Returns the vendor of the tape
     * @return the vendor of the tape
     */
    std::string getVendor() const;
    
    /**
    * Returns the capacity in bytes of the tape
    * @return the capacity in bytes of the tape
    */
    uint64_t getCapacityInBytes() const;
    
    /**
     * Report a drive status change
     */
    virtual void setDriveStatus(cta::common::dataStructures::DriveStatus status, const cta::optional<std::string> & reason = cta::nullopt);
    
    /**
     * Report a tape session statistics
     */
    virtual void setTapeSessionStats(const castor::tape::tapeserver::daemon::TapeSessionStats &stats);
    
    /**
     * Report a tape mounted event
     * @param logContext
     */
    virtual void setTapeMounted(log::LogContext &logContext) const;
    
    /**
     * Indicates that the disk thread of the mount was completed. This
     * will implicitly trigger the transition from DrainingToDisk to Up if necessary.
     */
    virtual void diskComplete();

    /**
     * Indicates that the tape thread of the mount was completed. This 
     * will implicitly trigger the transition from Unmounting to either Up or
     * DrainingToDisk, depending on the the disk thread's status.
     */
    virtual void tapeComplete();
    
    /**
     * Indicates that the we should cancel the mount (equivalent to diskComplete
     * + tapeComeplete).
     */
    virtual void abort();
    
    /**
     * Tests whether all threads are complete
     * @return true if both tape and disk are complete.
     */
    virtual bool bothSidesComplete();
    
    CTA_GENERATE_EXCEPTION_CLASS(SessionNotRunning);
    
    /**
     * Batch job factory
     * 
     * @param filesRequested the number of files requested
     * @param bytesRequested the number of bytes requested
     * @param logContext
     * @return a list of unique_ptr to the next retrieve jobs. The list is empty
     * when no more jobs can be found. Will return jobs (if available) until one
     * of the 2 criteria is fulfilled.
     */
    virtual std::list<std::unique_ptr<RetrieveJob>> getNextJobBatch(uint64_t filesRequested,
      uint64_t bytesRequested, log::LogContext &logContext);
    
    /**
     * Wait and complete reporting of a batch of jobs successes. The per jobs handling has
     * already been launched in the background using cta::RetrieveJob::asyncComplete().
     * This function will check completion of those async completes and then proceed
     * with any necessary common handling.
     *
     * @param successfulRetrieveJobs the jobs to report
     * @param logContext
     */
    virtual void flushAsyncSuccessReports(std::queue<std::unique_ptr<cta::RetrieveJob> > & successfulRetrieveJobs, cta::log::LogContext &logContext);
    
    
    /**
     * Creates a disk reporter for the retrieve job (this is a wrapper).
     * @param URL: report address
     * @return pointer to the reporter created.
     */
    disk::DiskReporter * createDiskReporter(std::string & URL);
    
    void setFetchEosFreeSpaceScript(const std::string & name);
    
    /**
     * Destructor.
     */
    virtual ~RetrieveMount() throw();

  private:

    /**
     * The database representation of this mount.
     */
    std::unique_ptr<cta::SchedulerDatabase::RetrieveMount> m_dbMount;
    
    /**
     * Internal tracking of the session completion
     */
    bool m_sessionRunning;
    
    /**
     * Internal tracking of the tape thread
     */
    bool m_tapeRunning;
    
    /**
     * Internal tracking of the disk thread
     */
    bool m_diskRunning;
    
    /** An initialized-once factory for archive reports (indirectly used by ArchiveJobs) */
    disk::DiskReporterFactory m_reporterFactory;
    
    /**
     * Internal tracking of the full disk systems. It is one strike out (for the mount duration).
     */
    std::set<std::string> m_fullDiskSystems;
    
    /**
     * A pointer to the file catalogue.
     */
    cta::catalogue::Catalogue &m_catalogue;
    
    /**
     * The name of the script that will be executed
     * to get the EOS free space 
     */
    std::string m_fetchEosFreeSpaceScript;
    
  }; // class RetrieveMount

} // namespace cta
