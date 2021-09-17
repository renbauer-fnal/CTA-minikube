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
#include "EosImportDirs.hpp"


namespace cta {
namespace migration {

struct EosImportDirsCmdLine {
  enum Command {
    INJECT,
    HELP,
    LIST_FAILURE_MODES,
    LIST_FAILURES,
    FORGET_FAILURES
  };

  static const std::string s_defaultConfigPath;

  std::string topLevelPath;
  std::string configPath;
  Command cmd;
  bool delta;
  bool retry;

  EosImportDirsCmdLine() : delta(false), retry(false) {}

  static void printUsage(std::ostream &os) {
    os <<
      "Usage:" << std::endl <<
      "    eos-import-dirs [--config <configuration_path>] --help | --failure-modes | --list-failures" << std::endl <<
      "    eos-import-dirs [--config <configuration_path>] [--delta|--retry|--forget-failures] <top_level_path>" << std::endl <<
      "Where:" << std::endl <<
      "    top_level_path" << std::endl <<
      "        The path of the top level directory to be imported into EOS." << std::endl <<
      "Options:" << std::endl <<
      "    -c, --config configuration_path" << std::endl <<
      "        The optional path to the configuration file." << std::endl <<
      "        The default value if not set is " << s_defaultConfigPath << std::endl <<
      "    -d, --delta" << std::endl <<
      "        Take input from CTADELTADIRSHELPER instead of CTADIRSHELPER." << std::endl <<
      "    -f, --failure-modes" << std::endl <<
      "        List the distinct error messages for insertions which failed." << std::endl <<
      "    -h, --help" << std::endl <<
      "        Prints this usage message." << std::endl <<
      "    -l, --list-failures" << std::endl <<
      "        Detailed listing of the directory insertions which failed." << std::endl <<
      "    -r, --retry" << std::endl <<
      "        Retry failed directory injections. (i.e. take input from CTADIRSFAILED instead of CTADIRSHELPER)" << std::endl <<
      "    -F, --forget-failures" << std::endl <<
      "        Remove all paths under <top_level_path> from CTADIRSFAILED." << std::endl;
  }

  static EosImportDirsCmdLine parseCmdLine(const int argc, char **argv) {
    EosImportDirsCmdLine cmdLine;

    static struct option longopts[] = {
      { "config",          required_argument, NULL, 'c' },
      { "delta",           no_argument,       NULL, 'd' },
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
    while ((opt = getopt_long(argc, argv, "c:fhlrF", longopts, NULL)) != -1) {
      switch (opt) {
      case 'c':
        cmdLine.configPath = optarg;
        break;
      case 'd':
        if(cmdLine.cmd != INJECT || cmdLine.retry) throw std::runtime_error("Invalid combination of options");
        cmdLine.delta = true;
        break;
      case 'r':
        if(cmdLine.cmd != INJECT || cmdLine.delta) throw std::runtime_error("Invalid combination of options");
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

    if(cmdLine.configPath.empty()) {
      cmdLine.configPath = s_defaultConfigPath;
    }

    // Calculate the number of non-option ARGV-elements
    const int nbArgs = argc - optind;

    // Check the number of arguments
    const int nbExpectedArgs = (cmdLine.cmd == INJECT || cmdLine.cmd == FORGET_FAILURES) ? 1 : 0;
    if (nbArgs != nbExpectedArgs) {
      std::ostringstream msg;
      msg << "Wrong number of command-line arguments: expected=" << nbExpectedArgs << " actual=" << nbArgs;
      throw std::runtime_error(msg.str());
    }

    if(nbExpectedArgs == 1) cmdLine.topLevelPath = argv[optind];

    return cmdLine;
  }
}; // struct EosImportDirsCmdLine

const std::string EosImportDirsCmdLine::s_defaultConfigPath("/etc/cta/castor-migration.conf");

}} // namespace cta::migration


int main(const int argc, char ** argv)
{
  using namespace cta::migration;
  try {
    EosImportDirsCmdLine cmdLine;

    try {
      cmdLine = EosImportDirsCmdLine::parseCmdLine(argc, argv);
    } catch (std::exception &ex) {
      std::cerr << "Failed to parse command-line: " << ex.what() << std::endl;
      EosImportDirsCmdLine::printUsage(std::cerr);
      return -1;
    }

    // Process the command
    switch(cmdLine.cmd) {
      case EosImportDirsCmdLine::HELP: {
        EosImportDirsCmdLine::printUsage(std::cerr);
        return 0;
      }
      case EosImportDirsCmdLine::LIST_FAILURE_MODES: {
        cta::migration::EosImportDirs importDirs(cmdLine.configPath);
        importDirs.listFailureModes();
        return 0;
      }
      case EosImportDirsCmdLine::LIST_FAILURES: {
        cta::migration::EosImportDirs importDirs(cmdLine.configPath);
        importDirs.listFailures();
        return 0;
      }
      case EosImportDirsCmdLine::FORGET_FAILURES: {
        cta::migration::EosImportDirs importDirs(cmdLine.configPath, cmdLine.topLevelPath);
        importDirs.forgetFailures();
        return 0;
      }
      case EosImportDirsCmdLine::INJECT: {
        cta::migration::EosImportDirs importDirs(cmdLine.configPath, cmdLine.topLevelPath, cmdLine.delta, cmdLine.retry);
        importDirs.preFlightCheck();
        importDirs.getStorageClasses();

        std::thread grpcInjectThread(&EosImportDirs::grpcInject, &importDirs);

        try {
          importDirs.checkRootDirExists();
          importDirs.select();
          importDirs.processDirs();
          importDirs.setDirsDone();
        } catch(...) {
          importDirs.setDirsDone();
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
