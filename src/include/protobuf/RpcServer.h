#pragma once

#include "tcpserver/tcpserver.h"

namespace google {
namespace protobuf {

class Service;
}  // namespace protobuf
}  // namespace google

namespace Tnet {

class RpcServer {
  public:
  RpcServer(EventLoop* loop, const InetAddress& listenAddr);

  void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

  void registerService(::google::protobuf::Service*);
  void start();

  private:
  void onConnection(const TcpConnectionPtr& conn);

  TcpServer server_;
  std::map<std::string, ::google::protobuf::Service*> services_;
};
}  // namespace Tnet
