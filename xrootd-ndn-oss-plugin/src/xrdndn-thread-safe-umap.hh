/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018 California Institute of Technology                        *
 *                                                                            *
 * Author: Catalin Iordache <catalin.iordache@cern.ch>                        *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.     *
 *****************************************************************************/

#ifndef XRDNDN_THREAD_SAFE_UMAP_HH
#define XRDNDN_THREAD_SAFE_UMAP_HH

#include <boost/thread/shared_mutex.hpp>
#include <unordered_map>

namespace xrdndn {
template <typename K, typename V> class ThreadSafeUMap {
  public:
    ThreadSafeUMap() = default;

    ~ThreadSafeUMap() { this->clear(); }

    const V &at(const K &key) {
        boost::unique_lock<boost::shared_mutex> lock(mutex_);
        return this->m_unorderedMap.at(key);
    }

    void clear() {
        boost::unique_lock<boost::shared_mutex> lock(mutex_);
        this->m_unorderedMap.clear();
    }

    size_t count(const K &key) {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        return this->m_unorderedMap.count();
    }

    size_t erase(const K &key) {
        boost::unique_lock<boost::shared_mutex> lock(mutex_);
        return this->m_unorderedMap.erase(key);
    }

    size_t hasKey(const K &key) {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        typename std::unordered_map<K, V>::const_iterator it =
            this->m_unorderedMap.find(key);

        return it == this->m_unorderedMap.end() ? 0 : 1;
    }

    void insert(std::pair<K, V> element) {
        boost::unique_lock<boost::shared_mutex> lock(mutex_);
        this->m_unorderedMap.insert(element);
    }

    size_t size() {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        return this->m_unorderedMap.size();
    }

    /* Iteration over this containter must be handled in a thread
     * safe manner by the user using the public mutex of this class. */

    typename std::unordered_map<K, V>::const_iterator begin() {
        return this->m_unorderedMap.begin();
    }

    /* Thread safety must be handled by end user using the mutex of this
     * class.*/
    typename std::unordered_map<K, V>::const_iterator end() {
        return this->m_unorderedMap.end();
    }

  public:
    mutable boost::shared_mutex mutex_;

  private:
    std::unordered_map<K, V> m_unorderedMap;
};
} // namespace xrdndn
#endif // XRDNDN_THREAD_SAFE_UMAP_HH