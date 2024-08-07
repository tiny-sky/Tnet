#pragma once

#include "tcpserver/tcpserver.h"

class EchoServer {
  public:
  EchoServer(Tnet::EventLoop* loop, const Tnet::InetAddress& listenAddr);

  void start();

  private:
  void onConnection(const Tnet::TcpConnectionPtr& conn);

  void onMessage(const Tnet::TcpConnectionPtr& conn, Tnet::Buffer* buf,
                 Tnet::Timestamp time);

  Tnet::TcpServer server_;
};
