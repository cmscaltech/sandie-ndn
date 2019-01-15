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

#ifndef XRDNDN_LRU_CACHE_HH
#define XRDNDN_LRU_CACHE_HH

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <list>
#include <unordered_map>

#include "../common/xrdndn-logger.hh"

namespace xrdndnproducer {
template <typename T> class LRUCacheEntry {
  public:
    LRUCacheEntry() {
        processingData = false;
        avoidEviction = false;
    }

    ~LRUCacheEntry() {
        boost::unique_lock<boost::mutex> lock(mutex);
        processingData = false;
        avoidEviction = false;
        cond.notify_all();
    }

    bool processingData;
    bool avoidEviction;

    mutable boost::mutex mutex;
    boost::condition_variable cond;

    std::shared_ptr<T> data;
};

template <typename K, typename V> class LRUCache {
    using List = std::list<K>; // For O(1) access time in cache
    using ListIterator = typename List::iterator;

    using Entry = std::shared_ptr<LRUCacheEntry<V>>;

    using Cache = std::unordered_map<K, std::pair<Entry, ListIterator>>;
    using CacheIterator = typename Cache::iterator;

  public:
    LRUCache(size_t cache_size, size_t cache_line_size)
        : m_cacheSz(cache_size), m_cacheLineSz(cache_line_size) {
        NDN_LOG_TRACE("Init LRU Cache.");
    }

    ~LRUCache() {
        NDN_LOG_TRACE("Dealloc LRU Cache.");
        m_List.clear();
        m_Cache.clear();
    }

  public:
    // Insert a new Cache Line in the Cache
    void insert(const K &key, Entry &value) {
        if (m_Cache.find(key) != m_Cache.end()) {
            NDN_LOG_TRACE("Line " << key << " already exists in cache.");
            return;
        }

        if (m_List.size() >= m_cacheSz) {
            size_t count = 0;
            while ((count < m_cacheLineSz) && (!m_List.empty())) {
                ++count;
                evict();
            }
        }

        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        ListIterator it = m_List.insert(m_List.end(), key);
        m_Cache.insert(std::make_pair(key, std::make_pair(value, it)));
    }

    bool hasKey(const K &key) {
        boost::shared_lock<boost::shared_mutex> lock(m_mutex);
        if (m_Cache.find(key) != m_Cache.end())
            return true;
        return false;
    }

    Entry &at(const K &key) {
        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        return m_Cache[key].first;
    }

    void resize(size_t cache_size) { m_cacheSz = cache_size; }

  private:
    // Remove Least Recently Used Cache line
    void evict() {
        if (m_List.empty()) {
            NDN_LOG_TRACE("List is empty in evict LRU Cache.");
            return;
        }

        boost::unique_lock<boost::shared_mutex> lock(m_mutex);
        CacheIterator it = m_Cache.find(m_List.front());

        if (it->second.first->avoidEviction) {
            m_List.splice(m_List.end(), m_List, it->second.second);
        } else {
            m_Cache.erase(it);
            m_List.pop_front();
        }
    }

  private:
    Cache m_Cache;
    List m_List;

    size_t m_cacheSz;
    size_t m_cacheLineSz;
    mutable boost::shared_mutex m_mutex;
};
} // namespace xrdndnproducer

#endif // XRDNDN_LRU_CACHE_HH