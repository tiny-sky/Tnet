#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "event/callbacks.h"
#include "tcpserver/inetaddress.h"
#include "util/buffer.h"
#include "util/macros.h"
#include "util/timestamp.h"

#include <boost/any.hpp>

namespace Tnet {

class Channel;
class EventLoop;
class Socket;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
  public:
  TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd,
                const InetAddress& localAddr, const InetAddress& peerAddr);
  ~TcpConnection();

  DISALLOW_COPY(TcpConnection)

  EventLoop* getLoop() const { return loop_; }
  const std::string& name() const { return name_; }
  const InetAddress& localAddress() const { return localAddr_; }
  const InetAddress& peerAddress() const { return peerAddr_; }

  bool connected() const { return state_ == kConnected; }

  // 发送数据
  void send(const std::string& buf);
  void send(Buffer* message);
  // 关闭连接
  void shutdown();
  void forceClose();
  void forceCloseInLoop();

  void setContext(const boost::any& context) { context_ = context; }

  const boost::any& getConstContext() const { return context_; }

  boost::any* getContext() { return &context_; }

  void setConnectionCallback(const ConnectionCallback& cb) {
    connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writeCompleteCallback_ = cb;
  }
  void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }
  void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,
                                std::size_t highWaterMark) {
    highWaterMarkCallback_ = cb;
    highWaterMark_ = highWaterMark;
  }

  // 连接建立
  void connectEstablished();
  // 连接销毁
  void connectDestroyed();

  private:
  enum StateE {
    kDisconnected,  // 已经断开连接
    kConnecting,    // 正在连接
    kConnected,     // 已连接
    kDisconnecting  // 正在断开连接
  };
  void setState(StateE state) { state_ = state; }

  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();

  void sendInLoop(const char* data, std::size_t len);
  void sendInLoop(const std::string& message);
  void shutdownInLoop();

  EventLoop* loop_;
  const std::string name_;
  std::atomic_int state_;
  // bool reading_;

  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;

  const InetAddress localAddr_;
  const InetAddress peerAddr_;

  ConnectionCallback connectionCallback_;        // 有新连接时的回调
  MessageCallback messageCallback_;              // 有读写消息时的回调
  WriteCompleteCallback writeCompleteCallback_;  // 消息发送完成以后的回调
  HighWaterMarkCallback highWaterMarkCallback_;
  CloseCallback closeCallback_;
  std::size_t highWaterMark_;

  // 数据缓冲区
  Buffer inputBuffer_;
  Buffer outputBuffer_;

  // For protobuf
  boost::any context_;
};
}  // namespace Tnet
