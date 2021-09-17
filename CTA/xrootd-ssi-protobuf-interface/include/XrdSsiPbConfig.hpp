/*!
 * @project        XRootD SSI/Protocol Buffer Interface Project
 * @brief          Read the XRootD configuration file into an object
 * @copyright      Copyright 2019 CERN
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

#pragma once

#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>

#include <XrdSsiPbException.hpp>

namespace XrdSsiPb {

//! Configuration option list type
typedef std::vector<std::string> optionlist_t;

/*!
 * Interface to the XRootD configuration file
 */
class Config
{
public:
   /*!
    * Default constructor: empty configuration
    */
   Config() {}

   /*!
    * Server-side constructor: construct from a configuration file
    */
   Config(const std::string &filename, const std::string &ns = "") :
      m_namespace(ns)
   {
      // Open the config file for reading
      std::ifstream file(filename);
      if(!file) {
         throw XrdSsiException("Failed to open " + filename);
      }

      // Parse the config file
      try {
         parse(file);
      } catch(std::exception &ex) {
        throw XrdSsiException("Failed to parse configuration file " + filename + ": " + ex.what());
      }
   }

   /*!
    * Client-side configuration: set config items from environment variables
    */
   void getEnv(const std::string &key, const char *var) {
      const char *val = getenv(var);

      if(val != nullptr) set(key, val);
   }

   /*!
    * Set (or override) config items manually
    */
   void set(std::string key, const std::string &val) {
      if(key.length() == 0) return;

      // Check whether key should be namespaced
      if(key.find('.') == std::string::npos && m_namespace.length() != 0) {
         key = m_namespace + "." + key;
      }

      // Tokenize the value
      std::stringstream sval(val);
      optionlist_t values = tokenize(sval);

      // Insert (key,value) pair into map
      if(!values.empty()) {
         m_configuration[key] = values;
      }
   }

   /*!
    * Set a namespace for configuration options
    */
   void setNamespace(const std::string &ns) {
      m_namespace = ns;
   }

   /*!
    * Get option list from config
    */
   const optionlist_t &getOptionList(std::string key) const {
      // Check whether key should be namespaced
      const std::string prefix = m_namespace + ".";
      if(m_namespace.length() != 0 && prefix.compare(0, prefix.length(), key)) {
         key = prefix + key;
      }

      // Look up the key and return the value if key is found
      auto it = m_configuration.find(key);
      return it == m_configuration.end() ? m_nulloptionlist : it->second;
   }

   /*!
    * Get a single option string value from config
    */
   std::pair<bool, std::string> getOptionValueStr(const std::string &key) const {
      auto optionlist = getOptionList(key);

      return optionlist.empty() ? std::make_pair(false, "")
                                : std::make_pair(true, optionlist.at(0));
   }

   /*!
    * Get a single option integer value from config
    *
    * Throws std::invalid_argument or std::out_of_range if the key exists but the value cannot be
    * converted to an integer
    */
   std::pair<bool, int> getOptionValueInt(const std::string &key) const {
      auto optionlist = getOptionList(key);

      return optionlist.empty() ? std::make_pair(false, 0)
                                : std::make_pair(true, std::stoi(optionlist.at(0)));
   }

   /*!
    * Get a single option bool value from config
    */
   std::pair<bool, bool> getOptionValueBool(const std::string &key) const {
      auto optionlist = getOptionList(key);

      if(optionlist.empty()) return std::make_pair(false, false);

      std::string strval = optionlist.at(0);
      std::transform(strval.begin(), strval.end(), strval.begin(), ::tolower);

           if(strval == "true")  return std::make_pair(true, true);
      else if(strval == "false") return std::make_pair(true, false);
      else {
         throw std::invalid_argument("\"" + optionlist.at(0) + "\" cannot be converted to Boolean type");
      }
   }

private:
   /*!
    * Parse config file
    */
   void parse(std::ifstream &file) {
      std::string line;

      while(std::getline(file, line)) {
         // Strip out comments
         auto pos = line.find('#');
         if(pos != std::string::npos) {
            line.resize(pos);
         }

         // Extract the key
         std::stringstream ss(line);
         std::string key;
         ss >> key;

         // Extract and store the config options
         if(!key.empty()) {
            optionlist_t values = tokenize(ss);

            if(!values.empty()) {
               m_configuration[key] = values;
            }
         }
      }
   }

   /*!
    * Tokenize a stringstream
    */
   optionlist_t tokenize(std::stringstream &input) {
      optionlist_t values;

      while(!input.eof()) {
         std::string value;
         input >> value;
         if(!value.empty()) values.push_back(value);
      }

      return values;
   }

   // Member variables

   const optionlist_t                  m_nulloptionlist;    //!< Empty option list returned when key not found
   std::string                         m_namespace;         //!< Namespace string to prepend to keys
   std::map<std::string, optionlist_t> m_configuration;     //!< Parsed configuration options
};

} // namespace XrdSsiPb
