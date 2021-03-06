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

/**
 * This program will create a VFS backend for the object store and populate
 * it with the minimum elements (the root entry). The program will then print out
 * the path the backend store and exit
 */

#include "common/Configuration.hpp"
#include "BackendFactory.hpp"
#include "common/log/DummyLogger.hpp"
#include "common/log/LogContext.hpp"
#include "BackendVFS.hpp"
#include "GenericObject.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char ** argv) {
  try {
    cta::log::DummyLogger dl("", "");
    std::unique_ptr<cta::objectstore::Backend> be;
    std::string objectName;
    if (3 == argc) {
      be.reset(cta::objectstore::BackendFactory::createBackend(argv[1], dl).release());
      objectName = argv[2];
    } else if (2 == argc ){
      cta::common::Configuration m_ctaConf("/etc/cta/cta-objectstore-tools.conf");
      be=std::move(cta::objectstore::BackendFactory::createBackend(m_ctaConf.getConfEntString("ObjectStore", "BackendPath", nullptr), dl));
      objectName = argv[1];
    } else {
      throw std::runtime_error("Wrong number of arguments: expected 1 or 2: [objectstoreURL] objectname");
    }
    // If the backend is a VFS, make sure we don't delete it on exit.
    // If not, nevermind.
    try {
      dynamic_cast<cta::objectstore::BackendVFS &>(*be).noDeleteOnExit();
    } catch (std::bad_cast &){}
    std::cout << "Object store path: " << be->getParams()->toURL() << std::endl 
        << "Object name: " << objectName << std::endl;
    cta::objectstore::GenericObject ge(objectName, *be);
    ge.fetchNoLock();
    std::cout << ge.dump() << std::endl;
  } catch (std::exception & e) {
    std::cerr << "Failed to dump object: "
        << std::endl << e.what() << std::endl;
  }
}
