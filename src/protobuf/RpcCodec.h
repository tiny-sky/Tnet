#pragma once

#include "protobuf/ProtobufCodecLite.h"
#include "util/timestamp.h"

namespace Tnet {

class Buffer;
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
extern const char rpctag[];// = "RPC0";

class RpcMessage;
using RpcMessagePtr = std::shared_ptr<RpcMessage>;

using RpcCodec =  ProtobufCodecLiteT<RpcMessage, rpctag>;
}  // namespace Tnet
