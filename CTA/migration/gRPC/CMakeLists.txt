# @project        The CERN Tape Archive (CTA)
# @copyright      Copyright(C) 2019-2021 CERN
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

find_package(xrootdclient REQUIRED)
find_package(Protobuf3 REQUIRED)

include_directories(${XRD_SSI_PB_DIR}/include ${XROOTD_INCLUDE_DIR} ${XROOTD_INCLUDE_DIR}/private
                    ${CMAKE_BINARY_DIR}/eos_cta
                    ${PROTOBUF3_INCLUDE_DIRS})

# Directory import tool
add_executable(eos-import-dirs EosImportDirsCmd.cpp EosImportDirs.cpp GrpcClient.cpp GrpcUtils.cpp)
target_link_libraries(eos-import-dirs EosMigration ${PROTOBUF3_LIBRARIES} ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY} ctacatalogue)
set_property(TARGET eos-import-dirs APPEND PROPERTY INSTALL_RPATH ${PROTOBUF3_RPATH})
set_property(TARGET eos-import-dirs APPEND PROPERTY INSTALL_RPATH ${ORACLE-INSTANTCLIENT_RPATH})

# File import tool
add_executable(eos-import-files EosImportFilesCmd.cpp EosImportFiles.cpp GrpcClient.cpp GrpcUtils.cpp)
target_link_libraries(eos-import-files EosMigration ${PROTOBUF3_LIBRARIES} ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY} ctacatalogue)
set_property(TARGET eos-import-files APPEND PROPERTY INSTALL_RPATH ${PROTOBUF3_RPATH})
set_property(TARGET eos-import-files APPEND PROPERTY INSTALL_RPATH ${ORACLE-INSTANTCLIENT_RPATH})

# Standalone test tools
#add_executable(grpc-utils-test GrpcUtilsTest.cpp GrpcUtils.cpp)

add_executable(eos-test-dir-inject EosTestDirInject.cpp GrpcClient.cpp GrpcUtils.cpp)
target_link_libraries(eos-test-dir-inject EosMigration ${PROTOBUF3_LIBRARIES} ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY})
set_property(TARGET eos-test-dir-inject APPEND PROPERTY INSTALL_RPATH ${PROTOBUF3_RPATH})

add_executable(eos-test-file-inject EosTestFileInject.cpp GrpcClient.cpp GrpcUtils.cpp)
target_link_libraries(eos-test-file-inject EosMigration ${PROTOBUF3_LIBRARIES} ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY})
set_property(TARGET eos-test-file-inject APPEND PROPERTY INSTALL_RPATH ${PROTOBUF3_RPATH})

install(TARGETS eos-import-dirs
                eos-import-files
                eos-test-dir-inject
                eos-test-file-inject
        DESTINATION usr/bin)
install(FILES ${CMAKE_SOURCE_DIR}/migration/gRPC/json-pretty-print.sh
        DESTINATION usr/bin)
install(FILES ${CMAKE_SOURCE_DIR}/migration/gRPC/castor-migration.conf.example
        DESTINATION etc/cta)
