#pragma once

#include <string>
#include <memory>
#include <folly/io/IOBuf.h>
#include <wangle/channel/Handler.h>

#define REQUEST_MAGIC       0x80
#define RESPONSE_MAGIC      0x81

/* Op codes.  Only the ones currently supported are defined */
#define GET_OP              0x00
#define SET_OP              0x01
#define DELETE_OP           0x04

/* Error codes */
#define STATUS_OK                                   0x0000
#define STATUS_KEY_NOT_FOUND                        0x0001
#define STATUS_KEY_EXISTS                           0x0002
#define STATUS_LARGE_VALUE                          0x0003
#define STATUS_INVALID_ARGS                         0x0004
#define STATUS_ITEM_NOT_STORED                      0x0005
#define STATUS_UNKNOWN_COMMAND                      0x0081
 
namespace memcache {

using Key = std::string;

/**
* @brief Packed header used in every request/response to memcache.
* Currently this header is 24 bytes
*/
struct __attribute__((packed)) Header {
    uint8_t     magic {0};
    uint8_t     opcode {0};
    uint16_t    keyLen {0};
    uint8_t     extrasLen {0};
    uint8_t     dataType {0};
    uint16_t    reserved {0};
    uint32_t    totalBodyLen {0};
    uint32_t    opaque {0};
    uint64_t    cas {0};
};

/* Forward declaration */
struct Message;
using MessagePtr = std::unique_ptr<Message>;

/**
* @brief Request/Response message structure
*/
struct Message {
    Header                              header;
    Key                                 key;
    std::unique_ptr<folly::IOBuf>       value;

    void deserialize(const folly::IOBuf &buf);
    std::unique_ptr<folly::IOBuf> serialize();

    /* Helper methods to create messages */
    static MessagePtr makeMessageWithStatus(uint8_t opcode, uint32_t status);
    static MessagePtr makeMessage(uint8_t opcode, bool isReq, const std::string &key);
    static MessagePtr makeMessage(uint8_t opcode, bool isReq,
                                  const std::string &key, const folly::IOBuf &value);
    static MessagePtr makeMessage(uint8_t opcode, bool isReq,
                                  const std::string &key, const std::string &value);
};

std::ostream& operator<< (std::ostream&, const Message&);

/**
* @brief Handles serialization from Message to IOBuf and deserialization from IOBuf to Message
*/
struct MessageSerializationHandler : wangle::Handler<std::unique_ptr<folly::IOBuf>, MessagePtr,
                                              MessagePtr, std::unique_ptr<folly::IOBuf>> 
{
    void read(Context* ctx, std::unique_ptr<folly::IOBuf> buf) override;
    folly::Future<folly::Unit> write(Context* ctx, MessagePtr msg) override;
};


}  // namespace memcache
