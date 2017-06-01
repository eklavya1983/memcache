#pragma once

#include <unordered_map>
#include <folly/io/async/EventBase.h>
#include <KVCache.h>

namespace memcache {

/**
 * @brief For now Shard is just a KVCache
 */
struct Shard : KVCache {};

/**
* @brief Cache entry
*/
struct CacheEntry {
    uint64_t                    version;
    std::list<Key>::iterator    lruPos;
    folly::IOBuf                value;
};

/**
 * @brief In memory implementation of shard that runs on event base.  All
 * operations on the shard are run on event base thread.  This alleviates
 * the need for locking and improves cache performance as well
 */
struct EmbeddedKvStoreShard : Shard {
    EmbeddedKvStoreShard(folly::EventBase *eb,
                         uint64_t maxCacheEntries);
    void init() override;
    folly::Future<MessagePtr> handleGet(MessagePtr req) override;
    folly::Future<MessagePtr> handleSet(MessagePtr req) override;
    folly::Future<MessagePtr> handleDelete(MessagePtr req) override;

 private:
    /* Eventbase on which all shard operations are performed */
    folly::EventBase                            *eb_;
    /* Maximum cache size */
    const uint64_t                              maxCacheEntries_;
    /* In-memory cache */
    std::unordered_map<Key, CacheEntry>         cache_;
    /* lru list.  front() points to most recently used item and back() points
     * to least recently used item
     */
    std::list<Key>                              lruList_;
};

}  // namespace memcache
