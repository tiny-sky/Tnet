#pragma once

#include <stddef.h>

#include <functional>
#include <memory>

namespace Tnet {

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using HighWaterMarkCallback =
    std::function<void(const TcpConnectionPtr &, std::size_t)>;

using MessageCallback =
    std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;

}  // namespace Tnet
