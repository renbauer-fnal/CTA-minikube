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

#include <gtest/gtest.h>
#include "Thread.hpp"
#include "ChildProcess.hpp"

/* This is a collection of multi process unit tests, which can (and should)
 be passed through helgrind, but not valgrind as the test framework creates
 many memory leaks in the child process. */

namespace threadedUnitTests {
  class emptyCleanup : public cta::threading::ChildProcess::Cleanup {
  public:

    virtual void operator ()() { };
  };

  class myOtherProcess : public cta::threading::ChildProcess {
  private:

    int run() {
      /* Just sleep a bit so the parent process gets a chance to see us running */
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 100*1000*1000;
      nanosleep(&ts, NULL);
      return 123;
    }
  };

  TEST(cta_threading, ChildProcess_return_value) {
    myOtherProcess cp;
    emptyCleanup cleanup;
    EXPECT_THROW(cp.exitCode(), cta::threading::ChildProcess::ProcessNeverStarted);
    EXPECT_NO_THROW(cp.start(cleanup));
    EXPECT_THROW(cp.exitCode(), cta::threading::ChildProcess::ProcessStillRunning);
    EXPECT_NO_THROW(cp.wait());
    ASSERT_EQ(123, cp.exitCode());
  }

  class myInfiniteSpinner : public cta::threading::ChildProcess {
  private:

    int run() {
      /* Loop forever (politely) */
      while (true) {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 10*1000*1000;
        nanosleep(&ts, NULL);
      }
      return 321;
    }
  };

  TEST(cta_threading, ChildProcess_killing) {
    myInfiniteSpinner cp;
    emptyCleanup cleanup;
    EXPECT_THROW(cp.kill(), cta::threading::ChildProcess::ProcessNeverStarted);
    EXPECT_NO_THROW(cp.start(cleanup));
    EXPECT_THROW(cp.exitCode(), cta::threading::ChildProcess::ProcessStillRunning);
    ASSERT_EQ(true, cp.running());
    EXPECT_NO_THROW(cp.kill());
    /* The effect is not immediate, wait a bit. */
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100*1000*1000;
    nanosleep(&ts, NULL);
    ASSERT_FALSE(cp.running());
    EXPECT_THROW(cp.exitCode(), cta::threading::ChildProcess::ProcessWasKilled);
  }
} // namespace threadedUnitTests
