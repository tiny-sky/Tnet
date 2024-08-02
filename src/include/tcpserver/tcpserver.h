#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "tcpserver/acceptor.h"
#include "util/buffer.h"
#include "event/callbacks.h"
#include "event/eventloop.h"
#include "tcpserver/inetaddress.h"
#include "util/macros.h"
#include "tcpserver/tcpconnection.h"
#include "thread/eventloopthreadpool.h"

namespace Tnet {

class TcpServer {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  enum Option {
    kNoReusePort,
    kReusePort,
  };

  TcpServer(EventLoop *loop, const InetAddress &listenAddr,
            const std::string &nameArg, Option option = kNoReusePort);
  ~TcpServer();

  DISALLOW_COPY(TcpServer)

  void setThreadInitCallback(const ThreadInitCallback &cb) {
    threadInitCallback_ = cb;
  }
  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }

  // 设置底层subloop的个数
  void setThreadNum(int numThreads);

  // 开启服务器监听
  void start();

 private:
  void newConnection(int sockfd, const InetAddress &peerAddr);
  void removeConnection(const TcpConnectionPtr &conn);
  void removeConnectionInLoop(const TcpConnectionPtr &conn);

  using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

  EventLoop *loop_;  // baseloop 用户自定义的loop

  const std::string ipPort_;
  const std::string name_;

  std::unique_ptr<Acceptor> acceptor_;  // 运行在mainloop 任务就是监听新连接事件

  std::shared_ptr<EventLoopThreadPool> threadPool_;  // one loop per thread

  ConnectionCallback connectionCallback_;  // 有新连接时的回调
  MessageCallback messageCallback_;        // 有读写事件发生时的回调
  WriteCompleteCallback writeCompleteCallback_;  // 消息发送完成后的回调

  ThreadInitCallback threadInitCallback_;  // loop线程初始化的回调

  std::atomic_int started_;

  int nextConnId_;
  ConnectionMap connections_;  // 保存所有的连接
};
}  // namespace Tnet
