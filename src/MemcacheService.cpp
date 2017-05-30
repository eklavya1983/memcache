#include <glog/logging.h>
#include <Types.h>
#include <wangle/channel/EventBaseHandler.h>
#include <wangle/codec/LengthFieldBasedFrameDecoder.h>

#include <wangle/codec/LineBasedFrameDecoder.h>
#include <wangle/codec/StringCodec.h>
#include <MemcacheService.h>

namespace memcache {

/**
* @brief Routes Message to appropriate shard.  Routes the response back to
* downstream in pipeline (typically EventBaseHandler->AsyncSocketHandler)
*/
struct ShardRouter :  HandlerAdapter<MessagePtr> {
    explicit ShardRouter(MemcacheService *service)
    : service_(service)
    {
    }

    void read(Context* ctx, MessagePtr req) override {
        VLOG(1) << *req;
        
        auto f = handle(std::move(req));
        f.then([this, ctx](MessagePtr resp) {
             VLOG(1) << *resp;

             write(ctx, std::move(resp));
        });
    }

    inline folly::Future<MessagePtr> handle(MessagePtr req) {
        auto shard = service_->lookupShard(req->key);
        switch (req->header.opcode) {
            case GET_OP:
                return shard->handleGet(std::move(req));
            case SET_OP:
                return shard->handleSet(std::move(req));
            case DELETE_OP:
                return shard->handleSet(std::move(req));
            default:
                return Message::makeMessageWithStatus(req->header.opcode, STATUS_UNKNOWN_COMMAND); 
        }
    }

 private:
    MemcacheService         *service_;
};

/**
* @brief Request/Response pipeline
*/
struct MemcachePipelineFactory : PipelineFactory<MemcachePipeline> {
    explicit MemcachePipelineFactory(MemcacheService *service)
    : service_(service)
    {}

    MemcachePipeline::Ptr newPipeline(std::shared_ptr<AsyncTransportWrapper> sock) {
        auto pipeline = MemcachePipeline::create();
        pipeline->addBack(AsyncSocketHandler(sock));
        pipeline->addBack(EventBaseHandler());
        pipeline->addBack(LengthFieldBasedFrameDecoder(4, UINT_MAX, 8, 12, 0, false));
        pipeline->addBack(MessageSerializationHandler());
        pipeline->addBack(ShardRouter(service_));
        pipeline->finalize();
        return pipeline;
    }

 private:
    MemcacheService *service_;
};

MemcacheService::MemcacheService(int port,
                                 int serverIOThreadsCount,
                                 int shardCount,
                                 int64_t maxCacheEntriesCount,
                                 int maxClients)
    : port_(port),
    shards_(shardCount)
{
    threadpool_ = std::make_shared<wangle::IOThreadPoolExecutor>(serverIOThreadsCount);
    for (auto & shard : shards_) {
        shard.reset(new EmbeddedKvStoreShard(threadpool_->getEventBase(),
                                             maxCacheEntriesCount));
    }
}

MemcacheService::~MemcacheService()
{

    threadpool_.reset();
    LOG(INFO) << "Destoryed threadpool";

    server_.stop();
    LOG(INFO) << "Stopped server";
}

void MemcacheService::init()
{
    server_.childPipeline(std::make_shared<MemcachePipelineFactory>(this));
    server_.bind(port_);
    LOG(INFO) << "Initialized server";
}

void MemcacheService::run()
{
    LOG(INFO) << "Running server";
    server_.waitForStop();
}

Shard* MemcacheService::lookupShard(const Key &key)
{
    static std::hash<std::string> hashFn;
    auto idx = hashFn(key) % shards_.size();
    return shards_[idx].get();
}

}  // namespace memcache
