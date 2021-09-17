# XRootD SSI/Protocol Buffer Interface Project
# Copyright 2018 CERN
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required (VERSION 2.6)

find_package(xrootdclient REQUIRED)
find_package(xrootd REQUIRED)
find_package(Protobuf3 REQUIRED)



#
# XRootD SSI
#
include_directories(${XROOTD_INCLUDE_DIR} ${XROOTD_INCLUDE_DIR}/private)

#
# XRootD SSI Protocol Buffer bindings
#
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/test/include)

#
# Compiled protocol buffers
#
include_directories(${CMAKE_BINARY_DIR}/test/protobuf ${PROTOBUF3_INCLUDE_DIRS})



#
# Test Client
#

add_executable(test-client TestClient.cpp)
target_link_libraries(test-client XrdSsi-4 XrdSsiLib XrdSsiPbTest XrdUtils)
set_property(TARGET test-client APPEND PROPERTY INSTALL_RPATH ${PROTOBUF3_RPATH})

add_executable(test-threads TestThreads.cpp)
target_link_libraries(test-threads XrdSsi-4 XrdSsiLib XrdSsiPbTest XrdUtils pthread)
set_property(TARGET test-threads APPEND PROPERTY INSTALL_RPATH ${PROTOBUF3_RPATH})

#
# XRootD SSI plugin for Test Server
#
add_library(XrdSsiTest MODULE TestServiceProvider.cpp TestRequestProc.cpp)
target_link_libraries(XrdSsiTest XrdSsi-4 XrdSsiLib XrdSsiPbTest)
set_property(TARGET XrdSsiTest APPEND PROPERTY INSTALL_RPATH ${PROTOBUF3_RPATH})

