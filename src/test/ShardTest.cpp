#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <Shard.h>


using namespace folly;
using namespace wangle;
using namespace memcache;


/**
 * @brief Basic test on lrucache
 */
TEST(Shard, lrucache)
{
    folly::EventBase eb;
    EmbeddedKvStoreShard shard(&eb, 3);
    shard.init();

    /* Add 3 entries */
    for (int i = 0; i < 3; i++) {
        auto req = Message::makeMessage(SET_OP, true,
                                        folly::sformat("key%d", i), "value");
        auto respFut = shard.handleSet(std::move(req));
        eb.loop();
        ASSERT_EQ(respFut.get()->header.reserved, STATUS_OK);
    }

    /* Add a new key, key3, key0 should be removed */
    auto req = Message::makeMessage(SET_OP, true,
                                    "key3", "value");
    auto key3SetRespFut = shard.handleSet(std::move(req));
    req = Message::makeMessage(GET_OP, true, "key0");
    auto key0GetRespFut = shard.handleGet(std::move(req));
    eb.loop();
    ASSERT_EQ(key3SetRespFut.get()->header.reserved, STATUS_OK);
    ASSERT_EQ(key0GetRespFut.get()->header.reserved, STATUS_KEY_NOT_FOUND);

}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    auto ret = RUN_ALL_TESTS();
    return ret;
}
