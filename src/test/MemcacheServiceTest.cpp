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
// typedef Pipeline<folly::IOBufQueue&, std::string> CacheClientPipeline;

// the handler for receiving messages back from the server
class CacheClientHandler : public HandlerAdapter<MessagePtr> {
 public:
  virtual void read(Context*, MessagePtr msg) override {
      LOG(INFO) << "received back: " << *msg;
  }
  virtual void readException(Context* ctx, exception_wrapper e) override {
    std::cout << exceptionStr(e) << std::endl;
    close(ctx);
  }
  virtual void readEOF(Context* ctx) override {
    std::cout << "EOF received :(" << std::endl;
    close(ctx);
  }
};

// the handler for receiving messages back from the server
class EchoHandler : public HandlerAdapter<std::string> {
 public:
  virtual void read(Context*, std::string msg) override {
    std::cout << "received back: " << msg;
  }
  virtual void readException(Context* ctx, exception_wrapper e) override {
    std::cout << exceptionStr(e) << std::endl;
    close(ctx);
  }
  virtual void readEOF(Context* ctx) override {
    std::cout << "EOF received :(" << std::endl;
    close(ctx);
  }
};

// chains the handlers together to define the response pipeline
class CacheClientPipelineFactory : public PipelineFactory<CacheClientPipeline> {
 public:
  CacheClientPipeline::Ptr newPipeline(std::shared_ptr<AsyncTransportWrapper> sock) {
    auto pipeline = CacheClientPipeline::create();
    pipeline->addBack(AsyncSocketHandler(sock));
    // pipeline->addBack(EventBaseHandler());
    /*
    pipeline->addBack(LineBasedFrameDecoder(8192, false));
    pipeline->addBack(StringCodec());
    pipeline->addBack(EchoHandler());
   */ 
    pipeline->addBack(LengthFieldBasedFrameDecoder(4, UINT_MAX, 8, 12, 0, false));
    pipeline->addBack(MessageSerializationHandler());
    pipeline->addBack(CacheClientHandler());
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
    auto msg = Message::makeMessage(GET_OP, true, "key1");
    auto rep = (*service)(std::move(msg));

    rep.then([&](MessagePtr val) {
         std::cout << *val;
         EventBaseManager::get()->getEventBase()->terminateLoopSoon();
    });
    EventBaseManager::get()->getEventBase()->loopForever();

    while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    server.reset();
    t.join();
}

#if 0
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

    ClientBootstrap<CacheClientPipeline> client;
    client.group(std::make_shared<wangle::IOThreadPoolExecutor>(1));
    client.pipelineFactory(std::make_shared<CacheClientPipelineFactory>());
    auto clientpipeline = client.connect(SocketAddress("::1", 8080)).get();
    auto msg = Message::makeMessage(GET_OP, true, "key1");
    clientpipeline->write(std::move(msg)).get();
    // clientpipeline->write("hello\r\n").get();

    while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    server.reset();
    t.join();
}
#endif

#if 0
TEST(Service, write)
{
    auto msg = Message::makeMessage(GET_OP, true, "key1");
    auto iobuf = msg->serialize();
    std::ofstream myFile ("data.bin", std::ios::out | std::ios::binary);
    myFile.write(reinterpret_cast<const char*>(iobuf->data()), iobuf->length());
    myFile.close();
}
#endif

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    auto ret = RUN_ALL_TESTS();
    return ret;
}
