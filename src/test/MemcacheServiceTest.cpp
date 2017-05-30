#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <MemcacheService.h>
#include <wangle/channel/EventBaseHandler.h>
#include <wangle/codec/LengthFieldBasedFrameDecoder.h>
#include <wangle/bootstrap/ClientBootstrap.h>
#include <wangle/service/ClientDispatcher.h>

#include <wangle/codec/LineBasedFrameDecoder.h>
#include <wangle/codec/StringCodec.h>
#include <fstream>


using namespace folly;
using namespace wangle;
using namespace memcache;

typedef Pipeline<folly::IOBufQueue&, MessagePtr> CacheClientPipeline;

// chains the handlers together to define the response pipeline
class CacheClientPipelineFactory : public PipelineFactory<CacheClientPipeline> {
 public:
  CacheClientPipeline::Ptr newPipeline(std::shared_ptr<AsyncTransportWrapper> sock) {
    auto pipeline = CacheClientPipeline::create();
    pipeline->addBack(AsyncSocketHandler(sock));
    pipeline->addBack(LengthFieldBasedFrameDecoder(4, UINT_MAX, 8, 12, 0, false));
    pipeline->addBack(MessageSerializationHandler());
    pipeline->finalize();
    return pipeline;
  }
};

template <typename Pipeline, typename Req, typename Resp>
class ClientServiceFactory : public ServiceFactory<Pipeline, Req, Resp> {
 public:
  class ClientService : public Service<Req, Resp> {
   public:
    explicit ClientService(Pipeline* pipeline) {
        dispatcher_.setPipeline(pipeline);
    }
    Future<Resp> operator()(Req request) override {
        return dispatcher_(std::move(request));
    }
   private:
    SerialClientDispatcher<Pipeline, Req, Resp> dispatcher_;
  };

  Future<std::shared_ptr<Service<Req, Resp>>> operator() (
      std::shared_ptr<ClientBootstrap<Pipeline>> client) override {
      return Future<std::shared_ptr<Service<Req, Resp>>>(
          std::make_shared<ClientService>(client->getPipeline()));
  }
};

TEST(MemcacheService, basicops)
{
    auto server = std::make_unique<memcache::MemcacheService>(8080,
                                                              2,
                                                              2,
                                                              4096,
                                                              2);
    server->init();
    std::thread t([&server]() {
          server->run();
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto client = std::make_shared<ClientBootstrap<CacheClientPipeline>>();
    ClientServiceFactory<CacheClientPipeline, MessagePtr, MessagePtr> serviceFactory;
    client->pipelineFactory(
        std::make_shared<CacheClientPipelineFactory>());
    SocketAddress addr("127.0.0.1", 8080);
    client->connect(addr);
    auto service = serviceFactory(client).value();

    /* Get of non-existent key should return STATUS_KEY_NOT_FOUND */ 
    auto req = Message::makeMessage(GET_OP, true, "key1");
    auto rep = (*service)(std::move(req));
    rep.then([&](MessagePtr val) {
         ASSERT_EQ(val->header.reserved, STATUS_KEY_NOT_FOUND); 
         EventBaseManager::get()->getEventBase()->terminateLoopSoon();
    });
    EventBaseManager::get()->getEventBase()->loopForever();

    /* Set should succeed */
    req = Message::makeMessage(SET_OP, true, "key1", "value1");
    rep = (*service)(std::move(req));
    rep.then([&](MessagePtr val) {
         ASSERT_EQ(val->header.reserved, STATUS_OK); 
         EventBaseManager::get()->getEventBase()->terminateLoopSoon();
    });
    EventBaseManager::get()->getEventBase()->loopForever();

    /* Get of valid key should return STATUS_OK */ 
    req = Message::makeMessage(GET_OP, true, "key1");
    rep = (*service)(std::move(req));
    rep.then([&](MessagePtr val) {
         ASSERT_EQ(val->header.reserved, STATUS_OK); 
         ASSERT_EQ(val->value->moveToFbString().toStdString(), "value1");
         EventBaseManager::get()->getEventBase()->terminateLoopSoon();
    });
    EventBaseManager::get()->getEventBase()->loopForever();

    /* Delete of non-existent key should return STATUS_KEY_NOT_FOUND */
    req = Message::makeMessage(DELETE_OP, true, "invalidkey");
    rep = (*service)(std::move(req));
    rep.then([&](MessagePtr val) {
         ASSERT_EQ(val->header.reserved, STATUS_KEY_NOT_FOUND); 
         EventBaseManager::get()->getEventBase()->terminateLoopSoon();
    });
    EventBaseManager::get()->getEventBase()->loopForever();

    /* Delete of valid key should return STATUS_OK */
    req = Message::makeMessage(DELETE_OP, true, "key1");
    rep = (*service)(std::move(req));
    rep.then([&](MessagePtr val) {
         ASSERT_EQ(val->header.reserved, STATUS_OK); 
         EventBaseManager::get()->getEventBase()->terminateLoopSoon();
    });
    EventBaseManager::get()->getEventBase()->loopForever();

    /* Ensure delete actually deleted */
    req = Message::makeMessage(DELETE_OP, true, "key1");
    rep = (*service)(std::move(req));
    rep.then([&](MessagePtr val) {
         ASSERT_EQ(val->header.reserved, STATUS_KEY_NOT_FOUND); 
         EventBaseManager::get()->getEventBase()->terminateLoopSoon();
    });
    EventBaseManager::get()->getEventBase()->loopForever();

    server.reset();
    t.join();
}

TEST(Types, DISABLED_serializeToFile)
{
    auto msg = Message::makeMessage(GET_OP, true, "key1");
    auto iobuf = msg->serialize();
    std::ofstream myFile ("data.bin", std::ios::out | std::ios::binary);
    myFile.write(reinterpret_cast<const char*>(iobuf->data()), iobuf->length());
    myFile.close();
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    auto ret = RUN_ALL_TESTS();
    return ret;
}
