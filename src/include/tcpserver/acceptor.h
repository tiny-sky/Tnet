#pragma once

#include <functional>

#include "event/channel.h"
#include "util/macros.h"
#include "tcpserver/socket.h"
#include "event/eventloop.h"

namespace Tnet {

class EventLoop;
class InetAddress;

class Acceptor {
 public:
  using NewConnectionCallback =
      std::function<void(int sockfd, const InetAddress &)>;

  Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
  ~Acceptor();

  DISALLOW_COPY(Acceptor)

  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    NewConnectionCallback_ = cb;
  }

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();

  EventLoop *loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback NewConnectionCallback_;
  bool listenning_;
};
}  // namespace Tnet
