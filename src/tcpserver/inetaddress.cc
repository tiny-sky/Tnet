#include "inetaddress.h"

#include <string.h>

namespace Tnet {

InetAddress::InetAddress(uint16_t port, std::string ip) {
  ::memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_port = ::htons(port);  // 本地字节序转为网络字节序
  addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const {
  // addr_
  char buf[64] = {0};
  ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
  return buf;
}

std::string InetAddress::toIpPort() const {
  // ip:port
  char ip[INET_ADDRSTRLEN] = {0};
  ::inet_ntop(AF_INET, &addr_.sin_addr, ip, sizeof(ip));
  uint16_t port = ::ntohs(addr_.sin_port);

  std::string ipPort = ip;
  ipPort += ":" + std::to_string(port);

  return ipPort;
}

uint16_t InetAddress::toPort() const {
  return ::ntohs(addr_.sin_port);
}
}  // namespace Tnet
