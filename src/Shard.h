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
    int32_t                 version;
    folly::IOBuf            value;
};

/**
 * @brief In memory implementation of shard that runs on event base.  All
 * operations on the shard are run on event base thread.  This alleviates
 * the need for locking and improves cache performance as well
 */
struct EmbeddedKvStoreShard : Shard {
    EmbeddedKvStoreShard(folly::EventBase *eb,
                         int64_t maxCacheEntries);
    void init() override;
    folly::Future<MessagePtr> handleGet(MessagePtr req) override;
    folly::Future<MessagePtr> handleSet(MessagePtr req) override;
    folly::Future<MessagePtr> handleDelete(MessagePtr req) override;

 private:
    folly::EventBase                            *eb_;
    const int64_t                               maxCacheEntries_;
    std::unordered_map<Key, CacheEntry>         cache_;
};

}  // namespace memcache
