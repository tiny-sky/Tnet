#pragma once

#include "tcpserver/tcpconnection.h"
#include "util/buffer.h"
#include "util/macros.h"

#include <google/protobuf/message.h>
#include <functional>
#include <memory>

namespace Tnet {
using MessagePtr = std::shared_ptr<google::protobuf::Message>;

class ProtobufCodec {
  public:
  enum ErrorCode {
    kNoError = 0,
    kInvalidLength,
    kCheckSumError,
    kInvalidNameLen,
    kUnknownMessageType,
    kParseError,
  };

  using ProtobufMessageCallback = std::function<void(
      const TcpConnectionPtr&, const MessagePtr&, Timestamp)>;
  using ErrorCallback = std::function<void(const TcpConnectionPtr&, Buffer*,
                                           Timestamp, ErrorCode)>;

  explicit ProtobufCodec(const ProtobufMessageCallback& messageCb)
      : messageCallback_(messageCb), errorCallback_(defaultErrorCallback) {}

  ProtobufCodec(const ProtobufMessageCallback& messageCb,
                const ErrorCallback& errorCb)
      : messageCallback_(messageCb), errorCallback_(errorCb) {}

  DISALLOW_COPY(ProtobufCodec);

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf,
                 Timestamp receiveTime);
  void send(const TcpConnectionPtr& conn,
            const google::protobuf::Message& message) {
    Buffer buf;
    fillEmptyBuffer(&buf, message);
    conn->send(&buf);
  }

  static void fillEmptyBuffer(Buffer* buf,
                              const google::protobuf::Message& message);

  private:
  static void defaultErrorCallback(const TcpConnectionPtr&, Buffer*, Timestamp,
                                   ErrorCode);

  ProtobufMessageCallback messageCallback_;
  ErrorCallback errorCallback_;
};

}  // namespace Tnet
