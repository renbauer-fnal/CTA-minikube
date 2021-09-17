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

#include <iostream>
#include "GrpcUtils.hpp"

void test(const std::string &p1, const std::string &p2, const std::string &path, const std::string &filename = "")
{
  auto d = eos::client::manglePathname(p1, p2, path, filename);
  std::cout << "(" << p1 << ", " << p2 << ") " << path << " -> " << d.pathname << " (" << d.basename << ")" << std::endl;
}

int main()
{
  std::string p1("");
  std::string p3("/");
  std::string p4("hello");
  std::string p5("/hello");
  std::string p6("hello/");
  std::string p7("/hello/");
  std::string p8("/hello/world/");
  eos::client::checkPrefix(p1);
  eos::client::checkPrefix(p3);
  eos::client::checkPrefix(p4);
  eos::client::checkPrefix(p5);
  eos::client::checkPrefix(p6);
  eos::client::checkPrefix(p7);
  eos::client::checkPrefix(p8);
  std::cout << p1 << std::endl;
  std::cout << p3 << std::endl;
  std::cout << p4 << std::endl;
  std::cout << p5 << std::endl;
  std::cout << p6 << std::endl;
  std::cout << p7 << std::endl;
  std::cout << p8 << std::endl;

  test("/", "/", "");
  test("/pre/", "/", "");
  test("/", "/pre/", "");
  test("/", "/", "/just/the/path");
  test("/prefix/", "/", "/prefix/just/the/path");
  test("/", "/just/", "/the/path");
  test("/one/", "/two/", "/one/rest/of/the/path");
  test("/substr/", "/two/", "/substring/rest/of/the/path");
  test("/prefix/is/the/path/", "/", "/prefix/is/the/path/");
  test("/prefix/is/the/path/", "/", "/prefix/is/the/path");
  test("/prefix/is/longer/than/the/path/", "/newprefix/", "/the/path");
  test("/", "/", "/path/with/no/filename/");
  test("/", "/", "/path/and/a/filename/", "filename");
  test("/", "/", "/path/and/a/filename", "filename");
}
