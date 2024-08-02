#include "tcpserver/acceptor.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcpserver/inetaddress.h"
#include "util/log.h"

namespace Tnet {
static int createNonblocking() {
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_ERROR("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__,
              __LINE__, errno);
  }
  return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr,
                   bool reuseport)
    : loop_(loop),
      acceptSocket_(createNonblocking()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false) {
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(reuseport);
  acceptSocket_.bindAddress(listenAddr);

  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
  acceptChannel_.disableAll();
  acceptChannel_.remove();
}

void Acceptor::listen() {
  loop_->assertInLoopThread();
  listenning_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
  loop_->assertInLoopThread();
  InetAddress peerAddr;
  int connfd = acceptSocket_.accept(&peerAddr);
  if (connfd >= 0) {
    if (NewConnectionCallback_) {
      NewConnectionCallback_(connfd, peerAddr);
    } else {
      ::close(connfd);
    }
  } else {
    LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__,
              errno);
    if (errno == EMFILE) {
      LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__,
                __LINE__);
    }
  }
}

}  // namespace Tnet
