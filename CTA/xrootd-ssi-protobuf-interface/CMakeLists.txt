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

# NOTE: This CMakeLists.txt is for compiling the test client/server only.
#       It is intended to be used only by the Continuous Integration
#       system and should not be included when this project is being
#       used as a git submodule. 

cmake_minimum_required (VERSION 2.6)

project(XrdSsiPbTest)

set(CMAKE_CXX_FLAGS "-fPIC -pedantic -Wall -Wextra -Werror -Wno-unused-parameter -Wno-long-long -fstack-protector-all -Wnon-virtual-dtor -std=c++1y -g")

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
include(GNUInstallDirs)
set(CMAKE_INSTALL_PREFIX "/")

#
# Compile test client/server
#

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/test/cmake)
add_subdirectory(test/protobuf)
add_subdirectory(test/src)

# Add custom targets for running tests
