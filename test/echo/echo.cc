#include "echo.h"

#include "util/log.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

EchoServer::EchoServer(Tnet::EventLoop* loop,
                       const Tnet::InetAddress& listenAddr)
    : server_(loop, listenAddr, "EchoServer") {
  server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&EchoServer::onMessage, this, _1, _2, _3));
}

void EchoServer::start() {
  server_.start();
}

void EchoServer::onConnection(const Tnet::TcpConnectionPtr& conn) {
  LOG_INFO("EchoServer - %s -> %s is %s",
           conn->peerAddress().toIpPort().c_str(),
           conn->localAddress().toIpPort().c_str(),
           (conn->connected() ? "UP" : "DOWN"));
}

void EchoServer::onMessage(const Tnet::TcpConnectionPtr& conn,
                           Tnet::Buffer* buf, Tnet::Timestamp time) {
  std::string msg(buf->retrieveAllAsString());
  LOG_INFO("%s echo %ld btyes, data received at %s", conn->name().c_str(),
           msg.size(), time.toString().c_str());
  conn->send(msg);
}