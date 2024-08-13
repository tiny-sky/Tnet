#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>

#include <string>

namespace Tnet {

class InetAddress {
 public:
  explicit InetAddress(uint16_t port = 9999, std::string ip = "127.0.0.1");
  explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

  sa_family_t family() const { return addr_.sin_family; }
  std::string toIp() const;
  std::string toIpPort() const;
  uint16_t toPort() const;

  const sockaddr_in *getSockAddr() const { return &addr_; }
  void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

 private:
  sockaddr_in addr_;
};

}  // namespace Tnet
