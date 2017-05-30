#include <gflags/gflags.h>
#include <MemcacheService.h>

DEFINE_int32(port, 8080, "server port");
DEFINE_int32(iothreads, 2, "Server socket IO threads");
DEFINE_int32(shards, 4, "Number of shards");
DEFINE_int32(cachesz, (1<<16), "Max number of cache entries");
DEFINE_int32(maxclients, 16, "Max number of clients");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  memcache::MemcacheService server(FLAGS_port,
                                   FLAGS_iothreads,
                                   FLAGS_shards,
                                   FLAGS_cachesz,
                                   FLAGS_maxclients);
  server.init();
  server.run();
  return 0;
}
