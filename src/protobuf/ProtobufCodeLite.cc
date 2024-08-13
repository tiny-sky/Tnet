#include "ProtobufCodecLite.h"
#include "tcpserver/tcpconnection.h"
#include "util/log.h"

#include <zlib.h>
#include "google/protobuf/message.h"
#include "protobuf/RpcCodec.h"

namespace Tnet {

const char rpctag[] = "RPC0";

void ProtobufCodecLite::send(const TcpConnectionPtr& conn,
                             const ::google::protobuf::Message& message) {
  Buffer buf;
  fillEmptyBuffer(&buf, message);
  conn->send(&buf);
}

void ProtobufCodecLite::fillEmptyBuffer(
    Buffer* buf, const google::protobuf::Message& message) {
  buf->append(tag_.data(), tag_.size());
  int byte_size = serializeToBuffer(message, buf);

  int32_t checkSum =
      checksum(buf->peek(), static_cast<int>(buf->readableBytes()));
  buf->appendInt32(checkSum);
  assert(buf->readableBytes() == tag_.size() + byte_size + kChecksumLen);
  int len = Endian::hostToNetwork32(static_cast<int32_t>(buf->readableBytes()));
  buf->prepend(&len, sizeof(len));
}

void ProtobufCodecLite::onMessage(const TcpConnectionPtr& conn, Buffer* buf,
                                  Timestamp receiveTime) {
  while (buf->readableBytes() >=
         static_cast<uint32_t>(kMinMessageLen + kHeaderLen)) {
    const int32_t len = buf->peekInt32();
    if (len > kMaxMessageLen || len < kMinMessageLen) {
      errorCallback_(conn, buf, receiveTime, kInvalidLength);
      break;
    } else if (buf->readableBytes() >=
               static_cast<std::size_t>(kHeaderLen + len)) {
      if (rawCb_ && !rawCb_(conn, std::string(buf->peek(), kHeaderLen + len),
                            receiveTime)) {
        buf->retrieve(kHeaderLen + len);
        continue;
      }
      MessagePtr message(prototype_->New());
      ErrorCode errorCode = parse(buf->peek() + kHeaderLen, len, message.get());
      if (errorCode == kNoError) {
        messageCallback_(conn, message, receiveTime);
        buf->retrieve(kHeaderLen + len);
      } else {
        errorCallback_(conn, buf, receiveTime, errorCode);
        break;
      }
    } else {
      break;
    }
  }
}

bool ProtobufCodecLite::parseFromBuffer(std::string buf,
                                        google::protobuf::Message* message) {
  return message->ParseFromArray(buf.data(), buf.size());
}

ProtobufCodecLite::ErrorCode ProtobufCodecLite::parse(
    const char* buf, int len, ::google::protobuf::Message* message) {
  ErrorCode error = kNoError;

  int32_t expectedCheckSum = parseInt32(buf + len - kChecksumLen);
  int32_t checkSum = checksum(buf, len - kChecksumLen);
  if (checkSum == expectedCheckSum) {
    if (memcmp(buf, tag_.data(), tag_.size()) == 0) {
      const char* data = buf + tag_.size();
      int32_t dataLen = len - kChecksumLen - static_cast<int>(tag_.size());
      if (parseFromBuffer(std::string(data, dataLen), message)) {
        error = kNoError;
      } else {
        error = kParseError;
      }
    } else {
      error = kUnknownMessageType;
    }
  } else {
    error = kCheckSumError;
  }

  return error;
}

int32_t ProtobufCodecLite::parseInt32(const char* buf) {
  int32_t be32 = 0;
  ::memcpy(&be32, buf, sizeof(be32));
  return Endian::networkToHost32(be32);
}

int32_t ProtobufCodecLite::checksum(const void* buf, int len) {
  return static_cast<int32_t>(
      ::adler32(1, static_cast<const Bytef*>(buf), len));
}

int ProtobufCodecLite::serializeToBuffer(
    const google::protobuf::Message& message, Buffer* buf) {
  int byte_size = message.ByteSizeLong();

  buf->ensureWritableBytes(byte_size + kChecksumLen);

  uint8_t* start = reinterpret_cast<uint8_t*>(buf->beginWrite());
  uint8_t* end = message.SerializeWithCachedSizesToArray(start);

  if (end - start != byte_size) {
    int new_size = message.ByteSizeLong();
    GOOGLE_CHECK_EQ(byte_size, new_size)
        << "Protocol message was modified concurrently during serialization.";
    GOOGLE_CHECK_EQ(new_size, static_cast<int>(end - start))
        << "Byte size calculation and serialization were inconsistent.  This "
           "may indicate a bug in protocol buffers or it may be caused by "
           "concurrent modification of the message.";
    GOOGLE_LOG(FATAL) << "This shouldn't be called if all the sizes are equal.";
  }
  buf->hasWritten(byte_size);
  return byte_size;
}

namespace {
const std::string kNoErrorStr = "NoError";
const std::string kInvalidLengthStr = "InvalidLength";
const std::string kCheckSumErrorStr = "CheckSumError";
const std::string kInvalidNameLenStr = "InvalidNameLen";
const std::string kUnknownMessageTypeStr = "UnknownMessageType";
const std::string kParseErrorStr = "ParseError";
const std::string kUnknownErrorStr = "UnknownError";
}  // namespace

const std::string& ProtobufCodecLite::errorCodeToString(ErrorCode errorCode) {
  switch (errorCode) {
    case kNoError:
      return kNoErrorStr;
    case kInvalidLength:
      return kInvalidLengthStr;
    case kCheckSumError:
      return kCheckSumErrorStr;
    case kInvalidNameLen:
      return kInvalidNameLenStr;
    case kUnknownMessageType:
      return kUnknownMessageTypeStr;
    case kParseError:
      return kParseErrorStr;
    default:
      return kUnknownErrorStr;
  }
}

void ProtobufCodecLite::defaultErrorCallback(const TcpConnectionPtr& conn,
                                             Buffer*, Timestamp,
                                             ErrorCode errorCode) {
  LOG_ERROR("defaultErrorCallback - %s", errorCodeToString(errorCode).c_str());
  if (conn && conn->connected()) {
    conn->shutdown();
  }
}

}  // namespace Tnet
