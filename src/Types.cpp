#include <Types.h>

namespace memcache {

void Message::deserialize(const folly::IOBuf &buf)
{
    const char *data = reinterpret_cast<const char*>(buf.data());

    static_assert(sizeof(Header) == 24, "Header must be 24 bytes");
    memcpy(&header, data, sizeof(Header));

    size_t keyOffset = sizeof(Header) + header.extrasLen;
    size_t valueOffset = keyOffset + header.keyLen; 
    size_t valueLen = header.totalBodyLen - (header.keyLen + header.extrasLen);
    key = std::string(&(data[keyOffset]), header.keyLen); 
    if (valueLen > 0) {
        value = folly::IOBuf::copyBuffer(data+valueOffset, valueLen);
    } else {
        value.reset();
    }
}

std::unique_ptr<folly::IOBuf> Message::serialize()
{
    size_t dataSz = sizeof(Header) + header.totalBodyLen;
    char* data = new char[dataSz];
    memcpy(data, &header, sizeof(Header));
    
    CHECK(header.extrasLen == 0);
    CHECK(header.keyLen == key.size());
    CHECK(header.totalBodyLen - (header.extrasLen + header.keyLen) == 
          (value ? value->length() : 0));
    if (key.size() > 0) {
        memcpy(&data[sizeof(Header)], key.data(), key.size());
    } 
    if (value && value->length() > 0) {
        memcpy(&data[sizeof(Header)+key.size()], value->data(), value->length());
    }
    return folly::IOBuf::copyBuffer(data, dataSz);
}

MessagePtr Message::makeMessageWithStatus(uint8_t opcode, uint32_t status)
{
    auto msg = std::make_unique<Message>();
    msg->header.magic = RESPONSE_MAGIC;
    msg->header.opcode = opcode;
    msg->header.reserved = status;

    return msg;
}

MessagePtr Message::makeMessage(uint8_t opcode, bool isReq, const std::string &key)
{
    auto msg = std::make_unique<Message>();
    msg->header.magic = isReq ? REQUEST_MAGIC : RESPONSE_MAGIC;
    msg->header.opcode = opcode;
    msg->header.keyLen = key.size();
    msg->header.totalBodyLen = key.size();
    
    msg->key = key;

    return msg;
}

MessagePtr Message::makeMessage(uint8_t opcode, bool isReq,
                                const std::string &key, const folly::IOBuf &value)
{
    auto msg = std::make_unique<Message>();
    msg->header.magic = isReq ? REQUEST_MAGIC : RESPONSE_MAGIC;
    msg->header.opcode = opcode;
    msg->header.keyLen = key.size();
    msg->header.totalBodyLen = key.size() + value.length();
    
    msg->key = key;
    msg->value = value.clone();

    return msg;
}

std::ostream& operator<< (std::ostream& os, const Message& msg)
{
    switch (msg.header.magic) {
        case REQUEST_MAGIC:
            os << " type:request";
            break;
        case RESPONSE_MAGIC:
            os << " type:response";
            break;
        default: 
            os << " type:Uknown";
            break;
    }
    switch (msg.header.opcode) {
        case GET_OP:
            os << " op:GET";
            break;
        case SET_OP:
            os << " op:SET";
            break;
        case DELETE_OP:
            os << " op:DELETE";
            break;
        default:
            os << " op:Unknown";
            break;
    }
    os << " key:" << msg.key;
    os << " valuesize: " << (msg.value ? msg.value->length() : 0);
    return os;
}

void MessageSerializationHandler::read(MessageSerializationHandler::Context* ctx,
                                       std::unique_ptr<folly::IOBuf> buf) {
    MessagePtr msg = std::make_unique<Message>();
    msg->deserialize(*buf);

    ctx->fireRead(std::move(msg));
}

folly::Future<folly::Unit> MessageSerializationHandler::write(
    MessageSerializationHandler::Context* ctx, MessagePtr msg)
{
    auto buf = msg->serialize();

    return ctx->fireWrite(std::move(buf));
}

}  // namespace memcache
