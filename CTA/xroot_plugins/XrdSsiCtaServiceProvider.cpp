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
#include "cta_frontend.pb.h"
#include "common/make_unique.hpp"
#include "common/log/SyslogLogger.hpp"
#include "common/log/StdoutLogger.hpp"
#include "common/log/FileLogger.hpp"
#include "common/log/LogLevel.hpp"
#include "common/utils/utils.hpp"
#include "rdbms/Login.hpp"
#include "version.h"
#include "XrdSsiCtaServiceProvider.hpp"
#include "XrdSsiPbAlert.hpp"
#include "XrdSsiPbConfig.hpp"
#include "XrdSsiPbService.hpp"

/*
 * Global pointer to the Service Provider object.
 *
 * This must be defined at library load time (i.e. it is a file-level global static symbol). When the
 * shared library is loaded, XRootD initialization fails if the appropriate symbol cannot be found (or
 * it is a null pointer).
 */
XrdSsiProvider *XrdSsiProviderServer = new XrdSsiCtaServiceProvider;

bool XrdSsiCtaServiceProvider::Init(XrdSsiLogger *logP, XrdSsiCluster *clsP, const std::string cfgFn,
                                    const std::string parms, int argc, char **argv)
{
   try {
      ExceptionThrowingInit(logP, clsP, cfgFn, parms, argc, argv);
      return true;
   } catch(XrdSsiPb::XrdSsiException &ex) {
      XrdSsiPb::Log::Msg(XrdSsiPb::Log::ERROR, LOG_SUFFIX, "XrdSsiCtaServiceProvider::Init(): XrdSsiPb::XrdSsiException ", ex.what());
   } catch(cta::exception::Exception &ex) {
      XrdSsiPb::Log::Msg(XrdSsiPb::Log::ERROR, LOG_SUFFIX, "XrdSsiCtaServiceProvider::Init(): cta::exception::Exception ", ex.what());
   } catch(std::exception &ex) {
      XrdSsiPb::Log::Msg(XrdSsiPb::Log::ERROR, LOG_SUFFIX, "XrdSsiCtaServiceProvider::Init(): std::exception ", ex.what());
   } catch(...) {
      XrdSsiPb::Log::Msg(XrdSsiPb::Log::ERROR, LOG_SUFFIX, "XrdSsiCtaServiceProvider::Init(): unknown exception");
   }

   // XRootD has a bug where if we return false here, it triggers a SIGABRT. Until this is fixed, exit(1) as a workaround.
   exit(1);

   return false;
}

void XrdSsiCtaServiceProvider::ExceptionThrowingInit(XrdSsiLogger *logP, XrdSsiCluster *clsP, const std::string &cfgFn,
                                                     const std::string &parms, int argc, char **argv)
{
   using namespace XrdSsiPb;
   using namespace cta;

   Log::Msg(XrdSsiPb::Log::INFO, LOG_SUFFIX, "Called Init(", cfgFn, ',', parms, ')');

   // Read CTA namespaced configuration options from XRootD config file
   Config config(cfgFn);

   // Set XRootD SSI Protobuf logging level
   auto loglevel = config.getOptionList("cta.log.ssi");
   if(!loglevel.empty()) {
      Log::SetLogLevel(loglevel);
   } else {
      Log::SetLogLevel("info");
   }

   // Instantiate the CTA logging system
   try {
      // Set the logger URL
      auto loggerURL = config.getOptionValueStr("cta.log.url");
      if(!loggerURL.first) loggerURL.second = "syslog:";
      const auto shortHostname = utils::getShortHostname();

      // Set the logger level
      int loggerLevel = log::INFO;
      auto loggerLevelStr = config.getOptionValueStr("cta.log.level");
      if(loggerLevelStr.first) loggerLevel = log::toLogLevel(loggerLevelStr.second);

      if (loggerURL.second == "syslog:") {
         m_log.reset(new log::SyslogLogger(shortHostname, "cta-frontend", loggerLevel));
      } else if (loggerURL.second == "stdout:") {
         m_log.reset(new log::StdoutLogger(shortHostname, "cta-frontend"));
      } else if (loggerURL.second.substr(0, 5) == "file:") {
         m_log.reset(new log::FileLogger(shortHostname, "cta-frontend", loggerURL.second.substr(5), loggerLevel));
      } else {
         throw exception::UserError(std::string("Unknown log URL: ") + loggerURL.second);
      }
   } catch(exception::Exception &ex) {
      std::string ex_str("Failed to instantiate object representing CTA logging system: ");
      throw exception::Exception(ex_str + ex.getMessage().str());
   }

   const std::list<log::Param> params = {log::Param("version", CTA_VERSION)};
   log::Logger &log = *m_log;

   // Initialise the Catalogue
   std::string catalogueConfigFile = "/etc/cta/cta-catalogue.conf";
   const rdbms::Login catalogueLogin = rdbms::Login::parseFile(catalogueConfigFile);
   auto catalogue_numberofconnections = config.getOptionValueInt("cta.catalogue.numberofconnections");
   if(!catalogue_numberofconnections.first) {
      throw exception::UserError("cta.catalogue.numberofconnections is not set in configuration file " + cfgFn);
   }
   const uint64_t nbArchiveFileListingConns = 2;

   {
     auto catalogueFactory = catalogue::CatalogueFactoryFactory::create(*m_log, catalogueLogin,
       catalogue_numberofconnections.second, nbArchiveFileListingConns);
     m_catalogue = catalogueFactory->create();
     try{
      m_catalogue->ping();
     } catch(cta::exception::Exception& ex){
       log::LogContext lc(*m_log);
       lc.log(cta::log::CRIT,ex.getMessageValue());
       throw ex;
     }
   }
   
   this->m_catalogue_conn_string = catalogueLogin.connectionString;

   // Initialise the Scheduler DB
   const std::string DB_CONN_PARAM = "cta.objectstore.backendpath";
   auto db_conn = config.getOptionValueStr(DB_CONN_PARAM);
   if(!db_conn.first) {
     throw exception::UserError(DB_CONN_PARAM + " is not set in configuration file " + cfgFn);
   }
   m_scheddb_init = cta::make_unique<SchedulerDBInit_t>("Frontend", db_conn.second, *m_log);
   m_scheddb      = m_scheddb_init->getSchedDB(*m_catalogue, *m_log);

   auto threadPoolSize = config.getOptionValueInt("cta.schedulerdb.numberofthreads");
   if (threadPoolSize.first) {
     m_scheddb->setThreadNumber(threadPoolSize.second);
   }
   m_scheddb->setBottomHalfQueueSize(25000);

   // Initialise the Scheduler
   m_scheduler = cta::make_unique<cta::Scheduler>(*m_catalogue, *m_scheddb, 5, 2*1000*1000);

   // Initialise the Frontend
   auto archiveFileMaxSize = config.getOptionValueInt("cta.archivefile.max_size_gb");
   m_archiveFileMaxSize = archiveFileMaxSize.first ? archiveFileMaxSize.second : 0; // GB
   m_archiveFileMaxSize *= 1024*1024*1024; // bytes

   // Get the repack buffer URL
   auto repackBufferURLConf = config.getOptionValueStr("cta.repack.repack_buffer_url");
   if(repackBufferURLConf.first){
     m_repackBufferURL = repackBufferURLConf.second;
   }

   // Get the endpoint for namespace queries
   auto nsConf = config.getOptionValueStr("cta.ns.config");
   if(nsConf.first) {
      setNamespaceMap(nsConf.second);
   } else {
      Log::Msg(XrdSsiPb::Log::WARNING, LOG_SUFFIX, "warning: 'cta.ns.config' not specified; namespace queries are disabled");
   }
  
   // All done
   log(log::INFO, std::string("cta-frontend started"), params);
}

void XrdSsiCtaServiceProvider::setNamespaceMap(const std::string &keytab_file)
{
   // Open the keytab file for reading
   std::ifstream file(keytab_file);
   if(!file) {
      throw cta::exception::UserError("Failed to open namespace keytab configuration file " + keytab_file);
   }

   // Parse the keytab line by line
   std::string line;
   for(int lineno = 0; std::getline(file, line); ++lineno) {
      // Strip out comments
      auto pos = line.find('#');
      if(pos != std::string::npos) {
         line.resize(pos);
      }

      // Parse one line
      std::stringstream ss(line);
      std::string diskInstance;
      std::string endpoint;
      std::string token;
      std::string eol;
      ss >> diskInstance >> endpoint >> token >> eol;

      // Ignore blank lines, all other lines must have exactly 3 elements
      if(token.empty() || !eol.empty()) {
         if(diskInstance.empty() && endpoint.empty() && token.empty()) continue;
         throw cta::exception::UserError("Could not parse namespace keytab configuration file line " + std::to_string(lineno) + ": " + line);
      }
      m_namespaceMap.insert(std::make_pair(diskInstance, cta::Namespace(endpoint, token)));
   }
}

XrdSsiService* XrdSsiCtaServiceProvider::GetService(XrdSsiErrInfo &eInfo, const std::string &contact, int oHold)
{
   XrdSsiPb::Log::Msg(XrdSsiPb::Log::INFO, LOG_SUFFIX, "Called GetService(", contact, ',', oHold, ')');

   XrdSsiService *ptr = new XrdSsiPb::Service<cta::xrd::Request, cta::xrd::Response, cta::xrd::Alert>;

   return ptr;
}

XrdSsiProvider::rStat XrdSsiCtaServiceProvider::QueryResource(const char *rName, const char *contact)
{
   // We only have one resource

   XrdSsiProvider::rStat resourcePresence = (strcmp(rName, "/ctafrontend") == 0) ?
                                            XrdSsiProvider::isPresent : XrdSsiProvider::notPresent;

   XrdSsiPb::Log::Msg(XrdSsiPb::Log::INFO, LOG_SUFFIX, "QueryResource(", rName, "): ",
                     ((resourcePresence == XrdSsiProvider::isPresent) ? "isPresent" : "notPresent"));

   return resourcePresence;
}
