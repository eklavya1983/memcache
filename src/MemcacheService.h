#pragma once
#include <wangle/bootstrap/ServerBootstrap.h>
#include <wangle/channel/AsyncSocketHandler.h>

#include <Shard.h>

namespace memcache {

using namespace folly;
using namespace wangle;

typedef Pipeline<IOBufQueue&, MessagePtr> MemcachePipeline;

/**
 * @brief Main container class for memcached service.  
 */
struct MemcacheService {
    MemcacheService(int port,
                    int shardThreadsCount,
                    int shardCount,
                    int64_t maxCacheEntriesCount,
                    int maxClients);
    virtual ~MemcacheService();

    /**
     * @brief Initilazes the service
     */
    void init();

    /**
     * @brief Runs the service
     */
    void run();

    /**
     * @brief Looks up shard associated with key and returns it.
     *
     * @return Associated shard or null if shard for key doesn't exist
     */
    Shard* lookupShard(const Key &key);

 private:
    int                                             port_;
    std::shared_ptr<wangle::IOThreadPoolExecutor>   threadpool_;
    ServerBootstrap<MemcachePipeline>               server_;
    std::vector<std::unique_ptr<Shard>>             shards_;
};
}  // namespace memcache
