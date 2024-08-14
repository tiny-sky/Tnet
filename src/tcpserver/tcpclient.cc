#include "tcpserver/tcpclient.h"

#include "event/callbacks.h"
#include "event/eventloop.h"
#include "tcpserver/connector.h"
#include "tcpserver/socket.h"
#include "util/log.h"

#include <stdio.h>

namespace {

void removeConnection(Tnet::EventLoop* loop,
                      const Tnet::TcpConnectionPtr& conn) {
  loop->queueInLoop(std::bind(&Tnet::TcpConnection::connectDestroyed, conn));
}
}  // namespace

namespace Tnet {

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr,
                     const std::string& nameArg)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr)),
      name_(nameArg),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      retry_(false),
      connect_(true),
      nextConnId_(1) {
  connector_->setNewConnectionCallback(
      std::bind(&TcpClient::newConnection, this, _1));

  LOG_INFO("Create TcpClient -> [%s]", name_.c_str());
}

TcpClient::~TcpClient() {
  LOG_INFO("TcpClient::~TcpClient[%s] - connector %p", name_.c_str(), connector_.get());
  TcpConnectionPtr conn;
  bool unique = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }
  if (conn) {
    CloseCallback cb = std::bind(&::removeConnection, loop_, _1);
    loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
    if (unique) {
      conn->forceClose();
    }
  } else {
    connector_->stop();
  }
}

void TcpClient::connect() {
  LOG_INFO("TcpClient::connect[%s] - connecting to %s", name_.c_str(),
           connector_->serverAddress().toIpPort().c_str());
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect() {
  connect_ = false;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_) {
      connection_->shutdown();
    }
  }
}

void TcpClient::stop() {
  connect_ = false;
  connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
  loop_->assertInLoopThread();
  InetAddress peerAddr(Socket::getPeerAddr(sockfd));
  char buf[32];
  snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toIpPort().c_str(),
           nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;

  InetAddress localAddr(Socket::getLocalAddr(sockfd));

  TcpConnectionPtr conn = std::make_shared<TcpConnection>(
      loop_, connName, sockfd, localAddr, peerAddr);

  LOG_INFO("Create TcpConnection : sockfd -> [%d]",sockfd);
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1));

  {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_ = conn;
  }
  conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
  loop_->assertInLoopThread();
  assert(loop_ == conn->getLoop());

  {
    std::lock_guard<std::mutex> lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  if (retry_ && connect_) {
    LOG_INFO("TcpClient::connect[%s] - Reconnecting to &%s", name_.c_str(),
             connector_->serverAddress().toIpPort().c_str());
    connector_->restart();
  }
}

}  // namespace Tnet
