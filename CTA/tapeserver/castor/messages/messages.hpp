/*
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2003-2021 CERN
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

#include "castor/messages/Constants.hpp"
#include "castor/messages/Exception.pb.h"
#include "castor/messages/Frame.hpp"
#include "castor/messages/Header.pb.h"
#include "castor/tape/tapeserver/daemon/Constants.hpp"
#include "common/exception/Exception.hpp"

#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>

namespace castor {
namespace messages {
  
/**
 * Function to compute inn Base64 the SHA1 of a given buffer
 * @param data The data
 * @param len, the length of the buffer
 * @return the base64 sha1 of the serialized buffer
 */
std::string computeSHA1Base64(const std::string &data);

/**
 * Function to compute inn Base64 the SHA1 of a given ProtoBuff message.
 * All fields must be set because we are going to serialize the message.
 * @param msg
 * @return the base64 sha1 of the serialized body
 */
std::string computeSHA1Base64(const google::protobuf::Message& msg);

/**
 * Function to compute inn Base64 the SHA1 of a given buffer
 * @param data The data
 * @param len, the length of the buffer
 * @return the base64 sha1 of the serialized buffer
 */
std::string computeSHA1Base64(void const* const data, const int len);

/**
 * Header factory which pre fill several fields
 * magic(TPMAGIC)
 * protocoltype(protocolType);
 * protocolversion(protocolVersion;
 * bodyhashtype("SHA1");
 * bodysignaturetype("SHA1");
 *  After, the only  fields left are reqtype, bodyhashvalue and bodyhashsignature
 * @return The header
 */
template <int magic, int protocolType, int protocolVersion>
castor::messages::Header genericPreFillHeader(){
  castor::messages::Header header;
  header.set_magic(magic);
  header.set_protocoltype(protocolType);
  header.set_protocolversion(protocolVersion);
  header.set_bodyhashtype("SHA1");
  header.set_bodysignaturetype("SHA1");
  return header;
}

/**
 * Return genericPreFillHeader() with
 * protocolType = <protocolType::Tape
 * protocolVersion = protocolVersion::prototype
 * @return 
 */
Header protoTapePreFillHeader();

} // namespace messages
} // namespace castor
