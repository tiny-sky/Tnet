#include "tcpserver/tcpserver.h"

#include <string.h>
#include "assert.h"

#include <functional>

#include "tcpserver/tcpconnection.h"
#include "util/log.h"

namespace Tnet {

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
  if (loop == nullptr) {
    LOG_ERROR("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
  }
  return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr,
                     const std::string& nameArg, Option option)
    : loop_(CheckLoopNotNull(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1),
      started_(0) {
  acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
}

TcpServer::~TcpServer() {
  for (auto& item : connections_) {
    TcpConnectionPtr conn(item.second);
    item.second.reset();

    conn->getLoop()->runInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
  }
}

void TcpServer::setThreadNum(int numThreads) {
  threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听
void TcpServer::start() {
  if (started_++ == 0)  // 防止一个TcpServer对象被start多次
  {
    threadPool_->start(threadInitCallback_);  // 启动底层的loop线程池
    assert(!acceptor_->listenning());
    loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
  // 轮询算法 选择一个subLoop 来管理connfd对应的channel
  EventLoop* ioLoop = threadPool_->getNextLoop();
  char buf[64] = {0};
  snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;

  sockaddr_in local;
  ::memset(&local, 0, sizeof(local));
  socklen_t addrlen = sizeof(local);
  if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0) {
    LOG_ERROR("sockets::getLocalAddr");
  }

  InetAddress localAddr(local);
  TcpConnectionPtr conn = std::make_shared<TcpConnection>(
      ioLoop, connName, sockfd, localAddr, peerAddr);

  LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n",
           name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

  connections_[connName] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);

  // close down
  conn->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

  ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
  LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
           name_.c_str(), conn->name().c_str());

  connections_.erase(conn->name());
  EventLoop* ioLoop = conn->getLoop();
  ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

}  // namespace Tnet
