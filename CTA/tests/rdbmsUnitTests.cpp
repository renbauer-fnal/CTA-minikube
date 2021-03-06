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

#include "catalogue/CatalogueFactoryFactory.hpp"
#ifdef STDOUT_LOGGING
#include "common/log/StdoutLogger.hpp"
#else
#include "common/log/DummyLogger.hpp"
#endif
#include "rdbms/Login.hpp"
#include "tests/GlobalCatalogueFactoryForUnitTests.hpp"
#include "tests/RdbmsUnitTestsCmdLineArgs.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>

/**
 * Prints the usage message of this unit-test program to the specified output stream.
 */
static void printUsage(std::ostream &os) {
  os <<
    "Usage:" << std::endl <<
    '\t' << "cta-rdbmsUnitTests [Google test options] databaseConnectionFile" << std::endl;
}

/**
 * Parses the specified command-line arguments.  This should be called after
 * Google test has consumed all of its command-line options from the
 * command-line.
 */
static RdbmsUnitTestsCmdLineArgs parseCmdLine(const int argc, char ** argv) {
  const int expectedNbArgs = 1;
  const int actualNbArgs = argc - 1;

  if(expectedNbArgs != actualNbArgs) {
    std::cerr << "Invalid number of command-line arguments: expected=" << expectedNbArgs << " actual=" << actualNbArgs
      << std::endl;
    std::cerr << std::endl;
    printUsage(std::cerr);
    exit(1);
  }

  RdbmsUnitTestsCmdLineArgs cmdLineArgs;
  cmdLineArgs.dbConfigPath = argv[1];

  return cmdLineArgs;
}

int main(int argc, char** argv) {
  try {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);

    // Google test will consume its options from the command-line and leave everything else
    g_cmdLineArgs = parseCmdLine(argc, argv);

    #ifdef STDOUT_LOGGING
    cta::log::StdoutLogger dummyLogger("stdout", "stdout");
    #else
    cta::log::DummyLogger dummyLogger("dummy", "dummy");
    #endif
    const auto login = cta::rdbms::Login::parseFile(g_cmdLineArgs.dbConfigPath);
    const uint64_t nbConns = 1;
    const uint64_t nbArchiveFileListingConns = 1;
    const uint64_t maxTriesToConnect = 1;
    auto catalogueFactory = cta::catalogue::CatalogueFactoryFactory::create(dummyLogger, login, nbConns,
      nbArchiveFileListingConns, maxTriesToConnect);
    g_catalogueFactoryForUnitTests = catalogueFactory.get();

    const int ret = RUN_ALL_TESTS();

    // Close standard in, out and error so that valgrind can be used with the
    // following command-line to track open file-descriptors:
    //
    //     valgrind --track-fds=yes
    close(0);
    close(1);
    close(2);

    return ret;
  } catch(cta::exception::Exception &ex) {
    std::cerr << "Aborting: Caught a cta::exception::Exception: " << ex.getMessage().str() << std::endl;
    return 1;
  } catch(std::exception &se) {
    std::cerr << "Aborting: Caught an std::exception: " << se.what() << std::endl;
    return 1;
  } catch(...) {
    std::cerr << "Aborting: Caught an unknown exception " << std::endl;
    return 1;
  }
}
