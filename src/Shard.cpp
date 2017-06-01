#include <Shard.h>

namespace memcache {

EmbeddedKvStoreShard::EmbeddedKvStoreShard(folly::EventBase *eb,
                                           uint64_t maxCacheEntries)
    : eb_(eb),
    maxCacheEntries_(maxCacheEntries)
{
}

void EmbeddedKvStoreShard::init()
{
}

folly::Future<MessagePtr> EmbeddedKvStoreShard::handleGet(MessagePtr req)
{
    return
    via(eb_).then([this, req = std::move(req)]() mutable {
        auto itr = cache_.find(req->key);         
        if (itr == cache_.end()) {
            return Message::makeMessageWithStatus(req->header.opcode, STATUS_KEY_NOT_FOUND); 
        } else if (req->header.cas && req->header.cas != itr->second.version) {
            return Message::makeMessageWithStatus(req->header.opcode, STATUS_KEY_NOT_FOUND); 
        } else {
            /* 
             * 1. Update lru list by moving key to front
             * 2. Update cach entry to point to lru key position
             */
            lruList_.erase(itr->second.lruPos);
            itr->second.lruPos = lruList_.insert(lruList_.begin(), req->key);

            return Message::makeMessage(req->header.opcode, false, req->key, itr->second.value);
        }
    });
}

folly::Future<MessagePtr> EmbeddedKvStoreShard::handleSet(MessagePtr req)
{
    return
    via(eb_).then([this, req = std::move(req)]() mutable {
        if (!req->value) {
            return Message::makeMessageWithStatus(req->header.opcode, STATUS_INVALID_ARGS);
        }

        auto itr = cache_.find(req->key);
        uint64_t retVersion;
        if (itr != cache_.end()) {
            /* Existing entry */
            itr->second.version++;
            retVersion = itr->second.version;

            /* 
             * 1. Update lru list by moving key to front
             * 2. Update cach entry to point to lru key position
             */
            lruList_.erase(itr->second.lruPos);
            itr->second.lruPos = lruList_.insert(lruList_.begin(), req->key);
        } else {
            /* New entry */
            if (cache_.size() == maxCacheEntries_) {
                /* Purge an entry from cache when cache is full */
                auto keyToRemove = lruList_.back();
                lruList_.pop_back();
                auto itr = cache_.find(keyToRemove);
                CHECK(itr != cache_.end());
                cache_.erase(itr);
            }
            /* Update lru list by adding key to front */
            auto lruEntryRef = lruList_.insert(lruList_.begin(), req->key);
            /* Add new entry into cache */
            retVersion = 1;
            cache_[req->key] = CacheEntry{retVersion, lruEntryRef, *(req->value)};
        }
        auto rep = Message::makeMessageWithStatus(req->header.opcode, STATUS_OK); 
        rep->header.cas = retVersion;
        return rep;
    });
}

folly::Future<MessagePtr> EmbeddedKvStoreShard::handleDelete(MessagePtr req)
{
    return
    via(eb_).then([this, req = std::move(req)]() mutable {
        auto itr = cache_.find(req->key);
        if (itr == cache_.end()) {
            return Message::makeMessageWithStatus(req->header.opcode, STATUS_KEY_NOT_FOUND); 
        } else {
            lruList_.erase(itr->second.lruPos);
            cache_.erase(itr);
            return Message::makeMessageWithStatus(req->header.opcode, STATUS_OK); 
        }
    });
}

}  // namespace memcache
