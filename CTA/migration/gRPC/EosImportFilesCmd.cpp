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

#include <getopt.h>
#include "EosImportFiles.hpp"


namespace cta {
namespace migration {

struct EosImportFilesCmdLine {
  enum Command {
    INJECT,
    HELP,
    LIST_FAILURE_MODES,
    LIST_FAILURES,
    FORGET_FAILURES
  };

  static const std::string s_defaultConfigPath;

  std::string configPath;
  std::string storageClass;
  Command cmd;
  bool retry;
  bool check;
  uint64_t skip;

  EosImportFilesCmdLine() : retry(false), check(false), skip(0) {}

  static void printUsage(std::ostream &os) {
    os <<
      "Usage:" << std::endl <<
      "    eos-import-files [--config <configuration_path>][--check][--skip <n>] [--help|--storage-class <storage_class>|--failure-modes|--list-failures|--retry|--forget-failures]" << std::endl <<
      "Options:" << std::endl <<
      "    -c, --config <configuration_path>" << std::endl <<
      "        The optional path to the configuration file." << std::endl <<
      "        The default value if not set is " << s_defaultConfigPath << std::endl <<
      "    -x, --check" << std::endl <<
      "        Check if each file exists in the EOS namespace before injecting (slower, but useful when resuming a failed import)" << std::endl <<
      "    -k, --skip <n>" << std::endl <<
      "        Skip the first n files before resuming import (useful when resuming a failed import)" << std::endl <<
      "    -f, --failure-modes" << std::endl <<
      "        List the distinct error messages for insertions which failed." << std::endl <<
      "    -F, --forget-failures" << std::endl <<
      "        Remove all paths from CTAFILESFAILED." << std::endl <<
      "    -h, --help" << std::endl <<
      "        Prints this usage message." << std::endl <<
      "    -l, --list-failures" << std::endl <<
      "        Detailed listing of the file insertions which failed." << std::endl <<
      "    -r, --retry" << std::endl <<
      "        Retry failed file injections. (i.e. take input from CTAFILESFAILED instead of CTAFILESHELPER)" << std::endl <<
      "    -s, --storage-class <storage_class>" << std::endl <<
      "        Filter the input to the specified storage class. Storage class can be the string name or numeric id." << std::endl;
  }

  static EosImportFilesCmdLine parseCmdLine(const int argc, char **argv) {
    EosImportFilesCmdLine cmdLine;

    static struct option longopts[] = {
      { "config",          required_argument, NULL, 'c' },
      { "check",           no_argument,       NULL, 'x' },
      { "skip",            required_argument, NULL, 'k' },
      { "storage-class",   required_argument, NULL, 's' },
      { "failure-modes",   no_argument,       NULL, 'f' },
      { "help",            no_argument,       NULL, 'h' },
      { "list-failures",   no_argument,       NULL, 'l' },
      { "retry",           no_argument,       NULL, 'r' },
      { "forget-failures", no_argument,       NULL, 'F' },
      { NULL, 0, NULL, 0}
    };

    // Prevent getopt() from printing an error message if it does not recognize
    // an option character
    opterr = 0;

    cmdLine.cmd = INJECT;
    int opt = 0;
    while ((opt = getopt_long(argc, argv, "c:k:s:xfhlrF", longopts, NULL)) != -1) {
      switch (opt) {
      case 'c':
        cmdLine.configPath = optarg;
        break;
      case 's':
        cmdLine.storageClass = optarg;
        break;
      case 'r':
        if(cmdLine.cmd != INJECT) throw std::runtime_error("Invalid combination of options");
        cmdLine.retry = true;
        break;
      case 'h':
        if(cmdLine.cmd != INJECT) throw std::runtime_error("Invalid combination of options");
        cmdLine.cmd = HELP;
        break;
      case 'f':
        if(cmdLine.cmd != INJECT) throw std::runtime_error("Invalid combination of options");
        cmdLine.cmd = LIST_FAILURE_MODES;
        break;
      case 'l':
        if(cmdLine.cmd != INJECT) throw std::runtime_error("Invalid combination of options");
        cmdLine.cmd = LIST_FAILURES;
        break;
      case 'F':
        if(cmdLine.cmd != INJECT) throw std::runtime_error("Invalid combination of options");
        cmdLine.cmd = FORGET_FAILURES;
        break;
      case 'k':
        if(cmdLine.cmd != INJECT) throw std::runtime_error("Invalid combination of options");
        cmdLine.skip = strtoul(optarg, NULL, 0);
        break;
      case 'x':
        if(cmdLine.cmd != INJECT) throw std::runtime_error("Invalid combination of options");
        cmdLine.check = true;
        break;
      case ':': // Missing parameter
      {
        std::ostringstream msg;
        msg << "The -" << (char) opt << " option requires a parameter";
        throw std::runtime_error(msg.str());
      }
      case '?': // Unknown option
      {
        std::ostringstream msg;
        if (0 == optopt) {
          msg << "Unknown command-line option";
        } else {
          msg << "Unknown command-line option: -" << (char) optopt;
        }
        throw std::runtime_error(msg.str());
      }
      default: {
        std::ostringstream msg;
        msg << "getopt_long returned the following unknown value: 0x" << std::hex << (int) opt;
        throw std::runtime_error(msg.str());
      }
      } // switch(opt)
    } // while getopt_long()

    if(cmdLine.cmd != INJECT && !cmdLine.storageClass.empty()) {
      throw std::runtime_error("Invalid combination of options");
    }

    if(cmdLine.configPath.empty()) {
      cmdLine.configPath = s_defaultConfigPath;
    }

    // Calculate the number of non-option ARGV-elements
    const int nbArgs = argc - optind;

    // Check the number of arguments
    const int nbExpectedArgs = 0;
    if (nbArgs != nbExpectedArgs) {
      std::ostringstream msg;
      msg << "Wrong number of command-line arguments: expected=" << nbExpectedArgs << " actual=" << nbArgs;
      throw std::runtime_error(msg.str());
    }

    return cmdLine;
  }
}; // struct EosImportFilesCmdLine

const std::string EosImportFilesCmdLine::s_defaultConfigPath("/etc/cta/castor-migration.conf");

}} // namespace cta::migration


int main(const int argc, char ** argv)
{
  using namespace cta::migration;
  try {
    EosImportFilesCmdLine cmdLine;

    try {
      cmdLine = EosImportFilesCmdLine::parseCmdLine(argc, argv);
    } catch (std::exception &ex) {
      std::cerr << "Failed to parse command-line: " << ex.what() << std::endl;
      EosImportFilesCmdLine::printUsage(std::cerr);
      return -1;
    }

    // Process the command
    switch(cmdLine.cmd) {
      case EosImportFilesCmdLine::HELP: {
        EosImportFilesCmdLine::printUsage(std::cerr);
        return 0;
      }
      case EosImportFilesCmdLine::LIST_FAILURE_MODES: {
        cta::migration::EosImportFiles importFiles(cmdLine.configPath);
        importFiles.listFailureModes();
        return 0;
      }
      case EosImportFilesCmdLine::LIST_FAILURES: {
        cta::migration::EosImportFiles importFiles(cmdLine.configPath);
        importFiles.listFailures();
        return 0;
      }
      case EosImportFilesCmdLine::FORGET_FAILURES: {
        cta::migration::EosImportFiles importFiles(cmdLine.configPath);
        importFiles.forgetFailures();
        return 0;
      }
      case EosImportFilesCmdLine::INJECT: {
        cta::migration::EosImportFiles importFiles(cmdLine.configPath, cmdLine.storageClass, cmdLine.retry, cmdLine.check, cmdLine.skip);
        importFiles.preFlightCheck();
        importFiles.getStorageClasses();

        std::thread grpcInjectThread(&EosImportFiles::grpcInject, &importFiles);

        try {
          importFiles.select();
          importFiles.processFiles();
          importFiles.setFilesDone();
        } catch(...) {
          importFiles.setFilesDone();
          grpcInjectThread.join();
          throw;
        }

        grpcInjectThread.join();
      }
    }
  } catch(std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return -1;
  }

  return 0;
}
