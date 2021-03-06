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


#pragma once

#include "common/threading/MutexLocker.hpp"
#include "common/threading/Thread.hpp"

namespace cta {
namespace threading {
/**
* A helper class managing a thread safe message counter
 * When C++11 will be used, just delete it to use std::atomic
*/
template <class T> struct AtomicCounter{
  AtomicCounter(T init = 0): m_val(init) {};
      T operator ++ () {
        MutexLocker ml(m_mutex);
        return ++m_val;
      }
      T operator ++ (int) {
        MutexLocker ml(m_mutex);
        return m_val++;
      }
      T operator -- () {
        MutexLocker ml(m_mutex);
        return --m_val;
      }
      operator T() const {
        MutexLocker ml(m_mutex);
        return m_val;
      }
      
     T getAndReset(){
       MutexLocker ml(m_mutex);
        T old =m_val;
        m_val=0;
        return old;
     }
    private:
      T m_val;
      mutable Mutex m_mutex;
};
} // namespace threading
} // namespace cta

