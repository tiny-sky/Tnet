#include "tcpserver/socket.h"

#include <fcntl.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

#include "tcpserver/inetaddress.h"
#include "util/log.h"

namespace Tnet {

Socket::~Socket() {
  ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr) {
  if (0 != ::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(),
                  sizeof(sockaddr_in))) {
    LOG_ERROR("bind sockfd:%d fail\n", sockfd_);
  }
}

void Socket::listen() {
  if (0 != ::listen(sockfd_, 1024)) {
    LOG_ERROR("listen sockfd:%d fail\n", sockfd_);
  }
}

int Socket::accept(InetAddress* peeraddr) {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);
  ::memset(&addr, 0, sizeof(addr));

  int connfd =
      ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd >= 0) {
    peeraddr->setSockAddr(addr);
  }
  return connfd;
}

void Socket::shutdownWrite() {
  if (::shutdown(sockfd_, SHUT_WR) < 0) {
    LOG_ERROR("shutdownWrite error");
  }
}

void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}
void Socket::setReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}
void Socket::setReusePort(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}
void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

int Socket::getSocketError(int sockfd) {
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof(optval));

  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  } else {
    return optval;
  }
}

bool Socket::isSelfConnect(int sockfd) {
  struct sockaddr_in localaddr = getLocalAddr(sockfd);
  struct sockaddr_in peeraddr = getPeerAddr(sockfd);
  return localaddr.sin_port == peeraddr.sin_port &&
         memcmp(&localaddr.sin_addr, &peeraddr.sin_addr,
                sizeof(localaddr.sin_addr)) == 0;
}

struct sockaddr_in Socket::getLocalAddr(int sockfd) {
  struct sockaddr_in localaddr;
  memset(&localaddr, 0, sizeof(localaddr));
  socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
  if (::getsockname(sockfd, reinterpret_cast<sockaddr*>(&localaddr), &addrlen) <
      0) {
    LOG_ERROR("sockets::getLocalAddr");
  }
  return localaddr;
}

struct sockaddr_in Socket::getPeerAddr(int sockfd) {
  struct sockaddr_in peeraddr;
  memset(&peeraddr, 0, sizeof(peeraddr));
  socklen_t addrlen = static_cast<socklen_t>(sizeof(peeraddr));
  if (::getpeername(sockfd, reinterpret_cast<sockaddr*>(&peeraddr), &addrlen) <
      0) {
    LOG_ERROR("sockets::getLocalAddr");
  }
  return peeraddr;
}
}  // namespace Tnet
