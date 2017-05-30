#pragma once

#include <folly/futures/Future.h>
#include <Types.h>

namespace memcache {

/**
 * @brief Key Value based cache interface
 */
struct KVCache {
    virtual ~KVCache() = default;
    virtual void init() = 0;
    virtual folly::Future<MessagePtr> handleGet(MessagePtr req) = 0;
    virtual folly::Future<MessagePtr> handleSet(MessagePtr req) = 0;
    virtual folly::Future<MessagePtr> handleDelete(MessagePtr req) = 0;
};

}  // namespace memcache
