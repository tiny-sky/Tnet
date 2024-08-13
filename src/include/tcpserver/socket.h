#pragma once

#include "util/macros.h"
#include <netinet/in.h>

namespace Tnet {

class InetAddress;

class Socket {
  public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}
  ~Socket();

  DISALLOW_COPY(Socket)

  int fd() const { return sockfd_; }
  void bindAddress(const InetAddress& localaddr);
  void listen();
  int accept(InetAddress* peeraddr);

  void shutdownWrite();

  void setTcpNoDelay(bool on);
  void setReuseAddr(bool on);
  void setReusePort(bool on);
  void setKeepAlive(bool on);

  static int getSocketError(int sockfd);
  static bool isSelfConnect(int sockfd);

  static struct sockaddr_in getLocalAddr(int sockfd);
  static struct sockaddr_in getPeerAddr(int sockfd);

  private:
  const int sockfd_;
};
}  // namespace Tnet
