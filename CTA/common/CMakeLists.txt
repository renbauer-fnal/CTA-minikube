# @project        The CERN Tape Archive (CTA)
# @copyright      Copyright(C) 2015-2021 CERN
# @license        This program is free software: you can redistribute it and/or modify
#                 it under the terms of the GNU General Public License as published by
#                 the Free Software Foundation, either version 3 of the License, or
#                 (at your option) any later version.
#
#                 This program is distributed in the hope that it will be useful,
#                 but WITHOUT ANY WARRANTY; without even the implied warranty of
#                 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#                 GNU General Public License for more details.
#
#                 You should have received a copy of the GNU General Public License
#                 along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required (VERSION 2.6)

find_package (binutils REQUIRED)
find_package (libattr REQUIRED)
find_package (libcap REQUIRED)
find_package (libuuid REQUIRED)
find_package(json-c REQUIRED)

add_subdirectory (exception)

include_directories (${XROOTD_INCLUDE_DIR})

#
# Compiled protocol buffers (used for ChecksumBlob serialization)
#
include_directories(${CMAKE_BINARY_DIR}/eos_cta ${PROTOBUF3_INCLUDE_DIRS})

set_source_files_properties(CRC.cpp PROPERTIES COMPILE_FLAGS -O2)

set (COMMON_LIB_SRC_FILES
  dataStructures/ActivitiesFairShareWeights.cpp
  dataStructures/AdminUser.cpp
  dataStructures/ArchiveFile.cpp
  dataStructures/ArchiveFileQueueCriteria.cpp
  dataStructures/ArchiveFileQueueCriteriaAndFileId.cpp
  dataStructures/ArchiveFileSummary.cpp
  dataStructures/ArchiveJob.cpp
  dataStructures/ArchiveRequest.cpp
  dataStructures/ArchiveRoute.cpp
  dataStructures/CancelRetrieveRequest.cpp
  dataStructures/DeleteArchiveRequest.cpp
  dataStructures/DesiredDriveState.cpp
  dataStructures/DiskFileInfo.cpp
  dataStructures/DriveState.cpp
  dataStructures/DriveStatus.cpp
  dataStructures/EntryLog.cpp
  dataStructures/FileRecycleLog.cpp
  dataStructures/FrontendReturnCode.cpp
  dataStructures/LifecycleTimings.cpp
  dataStructures/ListStorageClassRequest.cpp
  dataStructures/LogicalLibrary.cpp
  dataStructures/MountType.cpp
  dataStructures/MountPolicy.cpp
  dataStructures/OwnerIdentity.cpp
  dataStructures/QueueAndMountSummary.cpp
  dataStructures/ReadTestResult.cpp
  dataStructures/RepackInfo.cpp
  dataStructures/RepackQueueType.cpp
  dataStructures/RequesterGroupMountRule.cpp
  dataStructures/RequesterIdentity.cpp
  dataStructures/RequesterMountRule.cpp
  dataStructures/RetrieveFileQueueCriteria.cpp
  dataStructures/RetrieveJob.cpp
  dataStructures/RetrieveRequest.cpp
  dataStructures/SecurityIdentity.cpp
  dataStructures/StorageClass.cpp
  dataStructures/Tape.cpp
  dataStructures/TapeDrive.cpp
  dataStructures/TapeFile.cpp
  dataStructures/TapeLog.cpp
  dataStructures/TestSourceType.cpp
  dataStructures/UpdateFileInfoRequest.cpp
  dataStructures/UpdateFileStorageClassRequest.cpp
  dataStructures/VirtualOrganization.cpp
  dataStructures/WriteTestResult.cpp
  dataStructures/utils.cpp
  checksum/ChecksumBlob.cpp
  exception/AcceptConnectionInterrupted.cpp
  exception/AcsQueryVolumeCmd.cpp
  exception/Backtrace.cpp
  exception/BadAlloc.cpp
  exception/CommandLineNotParsed.cpp
  exception/DismountFailed.cpp
  exception/ForceDismountFailed.cpp
  exception/Errnum.cpp
  exception/Exception.cpp
  exception/InvalidArgument.cpp
  exception/InvalidConfigEntry.cpp
  exception/LostDatabaseConnection.cpp
  exception/Mismatch.cpp
  exception/MissingOperand.cpp
  exception/MountFailed.cpp
  exception/NoEntry.cpp
  exception/NonRetryableError.cpp
  exception/NoPortInRange.cpp
  exception/NotAnOwner.cpp
  exception/OutOfMemory.cpp
  exception/QueryVolumeFailed.cpp
  exception/RequestFailed.cpp
  exception/RetryableError.cpp
  exception/TimeOut.cpp
  exception/UserError.cpp
  exception/XrootCl.cpp
  json/object/JSONObject.cpp
  json/object/JSONCObject.cpp
  log/DummyLogger.cpp
  log/FileLogger.cpp
  log/LogContext.cpp
  log/Logger.cpp
  log/LogLevel.cpp
  log/Message.cpp
  log/Param.cpp
  log/PriorityMaps.cpp
  log/StringLogger.cpp
  log/SyslogLogger.cpp
  log/StdoutLogger.cpp
  log/TimingList.cpp
  priorities/DriveQuota.cpp
  priorities/MountCriteria.cpp
  priorities/UserGroup.cpp
  processCap/ProcessCap.cpp
  processCap/ProcessCapDummy.cpp
  processCap/SmartCap.cpp
  remoteFS/RemoteFileStatus.cpp
  remoteFS/RemotePath.cpp
  remoteFS/RemotePathAndStatus.cpp
  SmartFd.cpp
  SmartFILEPtr.cpp
  CRC.cpp
  threading/ChildProcess.cpp
  threading/CondVar.cpp
  threading/Daemon.cpp
  threading/Mutex.cpp
  threading/RWLock.cpp
  threading/RWLockRdLocker.cpp
  threading/RWLockWrLocker.cpp
  threading/SocketPair.cpp
  threading/System.cpp
  threading/Thread.cpp
  threading/Semaphores.cpp
  threading/SubProcess.cpp
  threading/Async.cpp
  utils/GetOptThreadSafe.cpp
  utils/Regex.cpp
  utils/utils.cpp
  utils/strerror_r_wrapper.cpp
  CreationLog.cpp
  Configuration.cpp
  ConfigurationFile.cpp
  SourcedParameter.cpp
  Timer.cpp
  optional.cpp)

add_library (ctacommon SHARED
  ${COMMON_LIB_SRC_FILES})
set_property(TARGET ctacommon PROPERTY SOVERSION "${CTA_SOVERSION}")
set_property(TARGET ctacommon PROPERTY   VERSION "${CTA_LIBVERSION}")

install (TARGETS ctacommon DESTINATION usr/${CMAKE_INSTALL_LIBDIR})

target_link_libraries (ctacommon
  XrdSsiPbEosCta
  pthread
  uuid
  z
  cap
  XrdCl
  json-c
)

set (COMMON_UNIT_TESTS_LIB_SRC_FILES
  checksum/ChecksumBlobTest.cpp
  ConfigurationFileTests.cpp
  SourcedParameterTests.cpp
  dataStructures/ArchiveFileTest.cpp
  dataStructures/LogicalLibraryTest.cpp
  dataStructures/StorageClassTest.cpp
  processCap/SmartCapTest.cpp
  log/FileLoggerTest.cpp
  log/LogContextTest.cpp
  log/LogLevelTest.cpp
  log/ParamTest.cpp
  log/SyslogLoggerTest.cpp
  log/StringLoggerTest.cpp
  remoteFS/RemotePathTest.cpp
  SmartFdTest.cpp
  SmartArrayPtrTest.cpp
  CRCTest.cpp
  threading/CondVarTest.cpp
  threading/DaemonTest.cpp
  threading/RWLockTest.cpp
  threading/SocketPairTest.cpp
  threading/ThreadingBlockingQTests.cpp
# threading/ThreadingMPTests.cpp is commented out because of errors caused by libust
  threading/ThreadingMTTests.cpp
  threading/ThreadingTests.cpp
  threading/AsyncTests.cpp
  threading/AtomicCounterTest.cpp
  utils/GetOptThreadSafeTest.cpp
  utils/RegexTest.cpp
  utils/UtilsTest.cpp
  optionalTest.cpp
  rangeTest.cpp
  json/test/JSONCTestObject.cpp
  json/test/JSONCObjectTest.cpp
)

add_library (ctacommonunittests SHARED
  ${COMMON_UNIT_TESTS_LIB_SRC_FILES})
set_property(TARGET ctacommonunittests PROPERTY SOVERSION "${CTA_SOVERSION}")
set_property(TARGET ctacommonunittests PROPERTY   VERSION "${CTA_LIBVERSION}")

install(TARGETS ctacommonunittests DESTINATION usr/${CMAKE_INSTALL_LIBDIR})
add_executable(mutexLtrace threading/MutexLtrace.cpp)
target_link_libraries(mutexLtrace ctacommon)
