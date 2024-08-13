#pragma once

#include "event/callbacks.h"
#include "util/macros.h"
#include "util/timestamp.h"

#include <google/protobuf/message.h>
#include <functional>
#include <memory>

namespace Tnet {
using MessagePtr = std::shared_ptr<google::protobuf::Message>;

/**
 * Field     Length  Content
 * 
 * size      4-byte  M+N+4
 * tag       M-byte  could be "RPC0", etc.
 * payload   N-byte
 * checksum  4-byte  adler32 of tag+payload
 */

class ProtobufCodecLite {
  public:
  const static int kHeaderLen = sizeof(int32_t);
  const static int kChecksumLen = sizeof(int32_t);
  const static int kMaxMessageLen = 64 * 1024 * 1024;

  enum ErrorCode {
    kNoError = 0,
    kInvalidLength,
    kCheckSumError,
    kInvalidNameLen,
    kUnknownMessageType,
    kParseError,
  };

  using RawMessageCallback =
      std::function<bool(const TcpConnectionPtr&, std::string, Timestamp)>;
  using ProtobufMessageCallback = std::function<void(
      const TcpConnectionPtr&, const MessagePtr&, Timestamp)>;
  using ErrorCallback = std::function<void(const TcpConnectionPtr&, Buffer*,
                                           Timestamp, ErrorCode)>;

  ProtobufCodecLite(const google::protobuf::Message* prototype,
                    std::string tagArg,
                    const ProtobufMessageCallback& messageCb,
                    const RawMessageCallback& rawCb = RawMessageCallback(),
                    const ErrorCallback& errorCb = defaultErrorCallback)
      : prototype_(prototype),
        tag_(tagArg),
        messageCallback_(messageCb),
        rawCb_(rawCb),
        errorCallback_(errorCb),
        kMinMessageLen(tagArg.size() + kChecksumLen) {}

  static void defaultErrorCallback(const TcpConnectionPtr&, Buffer*, Timestamp,
                                   ErrorCode);

  virtual ~ProtobufCodecLite() = default;

  const std::string& tag() const { return tag_; }

  void send(const TcpConnectionPtr& conn,
            const ::google::protobuf::Message& message);

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf,
                 Timestamp receiveTime);

  virtual bool parseFromBuffer(std::string buf,
                               google::protobuf::Message* message);
  virtual int serializeToBuffer(const google::protobuf::Message& message,
                                Buffer* buf);
  ErrorCode parse(const char* buf, int len,
                  ::google::protobuf::Message* message);
  int32_t parseInt32(const char* buf);

  static int32_t checksum(const void* buf, int len);
  void fillEmptyBuffer(Buffer* buf, const google::protobuf::Message& message);

  static const std::string& errorCodeToString(ErrorCode errorCode);

  private:
  const ::google::protobuf::Message* prototype_;
  const std::string tag_;
  ProtobufMessageCallback messageCallback_;
  RawMessageCallback rawCb_;
  ErrorCallback errorCallback_;
  const int kMinMessageLen;
};

template <typename MSG, const char* TAG, typename CODEC = ProtobufCodecLite>
class ProtobufCodecLiteT {
  public:
  using ConcreteMessagePtr = std::shared_ptr<MSG>;
  using ProtobufMessageCallback = std::function<void(
      const TcpConnectionPtr&, const ConcreteMessagePtr&, Timestamp)>;
  using ErrorCallback = ProtobufCodecLite::ErrorCallback;
  using RawMessageCallback = ProtobufCodecLite::RawMessageCallback;

  explicit ProtobufCodecLiteT(
      const ProtobufMessageCallback& messageCb,
      const RawMessageCallback& rawCb = RawMessageCallback(),
      const ErrorCallback& errorCb = ProtobufCodecLite::defaultErrorCallback)
      : messageCallback_(messageCb),
        codec_(&MSG::default_instance(), TAG,
               std::bind(&ProtobufCodecLiteT::onRpcMessage, this, _1, _2, _3),
               rawCb, errorCb) {}
  const std::string& tag() const { return codec_.tag(); }

  void send(const TcpConnectionPtr& conn, const MSG& message) {
    codec_.send(conn, message);
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf,
                 Timestamp receiveTime) {
    codec_.onMessage(conn, buf, receiveTime);
  }

  // internal
  void onRpcMessage(const TcpConnectionPtr& conn, const MessagePtr& message,
                    Timestamp receiveTime) {
    messageCallback_(conn, down_pointer_cast<MSG>(message), receiveTime);
  }

  void fillEmptyBuffer(Buffer* buf, const MSG& message) {
    codec_.fillEmptyBuffer(buf, message);
  }

  private:
  ProtobufMessageCallback messageCallback_;
  CODEC codec_;
};

}  // namespace Tnet
