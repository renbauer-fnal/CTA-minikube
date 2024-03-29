# @project        The CERN Tape Archive (CTA)
# @copyright      Copyright(C) 2021 CERN
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

find_package(Protobuf3 REQUIRED)

include_directories(${PROJECT_BINARY_DIR}/tapeserver)

PROTOBUF3_GENERATE_CPP(WDMsgSources WDMsgHeaders WatchdogMessage.proto)

SET_SOURCE_FILES_PROPERTIES(${WDMsgHeaders} PROPERTIES HEADER_FILE_ONLY TRUE)
SET_SOURCE_FILES_PROPERTIES(DriveHandler.cpp PROPERTIES OBJECT_DEPENDS ${WDMsgHeaders})

include_directories(${PROTOBUF3_INCLUDE_DIRS})
add_library(ctatapedaemon
  ${WDMsgSources}
  CommandLineParams.cpp
  DriveHandler.cpp
  DriveHandlerProxy.cpp
  MaintenanceHandler.cpp
  SignalHandler.cpp
  SubprocessHandler.cpp
  ProcessManager.cpp
  TapedConfiguration.cpp
  TapeDaemon.cpp
  TapedProxy.cpp
  TpconfigLine.cpp
  Tpconfig.cpp)

if(CMAKE_COMPILER_IS_GNUCC)
  if(GCC_VERSION_GE_4_8_0)
    foreach(CTATAPESERVERDAEMON_LIBRARY_SRC
      DriveHandler.cpp)
      set_property(SOURCE ${CTATAPESERVERDAEMON_LIBRARY_SRC}
        PROPERTY COMPILE_FLAGS " -Wno-unused-local-typedefs")
    endforeach(CTATAPESERVERDAEMON_LIBRARY_SRC)
  endif(GCC_VERSION_GE_4_8_0)
endif(CMAKE_COMPILER_IS_GNUCC)

target_link_libraries(ctatapedaemon
  ctatapesession
  ctaTapeServerDaemon
  SCSI
  TapeDrive
  File)

add_library(ctadaemonunittests SHARED
  TapedConfigurationTests.cpp
  TpconfigTests.cpp)
set_property(TARGET ctadaemonunittests PROPERTY SOVERSION "${CTA_SOVERSION}")
set_property(TARGET ctadaemonunittests PROPERTY   VERSION "${CTA_LIBVERSION}")

add_library(ctadaemonunittests-multiprocess SHARED
  ProcessManagerTests.cpp
  SignalHandlerTests.cpp)
set_property(TARGET ctadaemonunittests-multiprocess PROPERTY SOVERSION "${CTA_SOVERSION}")
set_property(TARGET ctadaemonunittests-multiprocess PROPERTY   VERSION "${CTA_LIBVERSION}")

target_link_libraries(ctadaemonunittests
  ctatapedaemon
  unitTestHelper)

target_link_libraries(ctadaemonunittests-multiprocess
  ctatapedaemon
  unitTestHelper)

install(TARGETS ctadaemonunittests DESTINATION usr/${CMAKE_INSTALL_LIBDIR})
install(TARGETS ctadaemonunittests-multiprocess DESTINATION usr/${CMAKE_INSTALL_LIBDIR})

install (FILES cta-taped.conf.example
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/cta
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
