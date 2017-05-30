#include <Shard.h>

namespace memcache {

EmbeddedKvStoreShard::EmbeddedKvStoreShard(folly::EventBase *eb,
                                           int64_t maxCacheEntries)
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
            itr->second.version++;
            retVersion = itr->second.version;
        } else {
            cache_[req->key] = CacheEntry{1, *(req->value)};
            retVersion = 1;
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
            cache_.erase(itr);
            return Message::makeMessageWithStatus(req->header.opcode, STATUS_OK); 
        }
    });
}

}  // namespace memcache
