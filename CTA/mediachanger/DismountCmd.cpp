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

#include "mediachanger/DismountCmd.hpp"

#include <getopt.h>
#include <iostream>
#include <memory>
 
//------------------------------------------------------------------------------
// constructor
//------------------------------------------------------------------------------
cta::mediachanger::DismountCmd::DismountCmd(
  std::istream &inStream, std::ostream &outStream, std::ostream &errStream,
  MediaChangerFacade &mc):
  CmdLineTool(inStream, outStream, errStream, mc) {
}

//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
cta::mediachanger::DismountCmd::~DismountCmd() {
  // Do nothing
}

//------------------------------------------------------------------------------
// exceptionThrowingMain
//------------------------------------------------------------------------------
int cta::mediachanger::DismountCmd::exceptionThrowingMain(const int argc,
  char *const *const argv) {
  try {
    m_cmdLine = DismountCmdLine(argc, argv);
  } catch(cta::exception::Exception &ex) {
    m_err << ex.getMessage().str() << std::endl;
    m_err << std::endl;
    m_err << m_cmdLine.getUsage() << std::endl;
    return 1;
  }

  // Display the usage message to standard out and exit with success if the
  // user requested help
  if(m_cmdLine.getHelp()) {
    m_out << m_cmdLine.getUsage();
    return 0;
  }

  // Setup debug mode to be on or off depending on the command-line arguments
  m_debugBuf.setDebug(m_cmdLine.getDebug());

  m_dbg << "VID        = " << m_cmdLine.getVid() << std::endl;
  m_dbg << "DRIVE_SLOT = " << m_cmdLine.getDriveLibrarySlot().str() <<
    std::endl;

  m_mc.dismountTape(m_cmdLine.getVid(), m_cmdLine.getDriveLibrarySlot());
  return 0;
}
