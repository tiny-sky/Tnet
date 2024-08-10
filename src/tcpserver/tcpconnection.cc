#include "tcpserver/tcpconnection.h"

#include <errno.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <functional>
#include <string>

#include "event/channel.h"
#include "event/eventloop.h"
#include "tcpserver/socket.h"
#include "util/log.h"

namespace Tnet {

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
  if (loop == nullptr) {
    LOG_ERROR("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
  }
  return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& nameArg,
                             int sockfd, const InetAddress& localAddr,
                             const InetAddress& peerAddr)
    : loop_(CheckLoopNotNull(loop)),
      name_(nameArg),
      state_(kConnecting),
      // reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) {
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

  LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", name_.c_str(),
           channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string& buf) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(buf.c_str(), buf.size());
    } else {
      void (TcpConnection::*fp)(const char* data, std::size_t len) =
          &TcpConnection::sendInLoop;
      loop_->runInLoop(std::bind(fp, this, buf.c_str(), buf.size()));
    }
  }
}

void TcpConnection::send(Buffer* buf) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    } else {
      void (TcpConnection::*fp)(const std::string& message) =
          &TcpConnection::sendInLoop;
      loop_->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
    }
  }
}

void TcpConnection::sendInLoop(const std::string& message) {
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const char* data, std::size_t len) {
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  std::size_t remaining = len;
  bool faultError = false;

  if (state_ == kDisconnected) {
    LOG_ERROR("disconnected, give up writing");
    return;
  }

  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    LOG_DEBUG("::write : %d -> %s", channel_->fd(), data);
    nwrote = ::write(channel_->fd(), data, len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_) {
        loop_->queueInLoop(
            std::bind(writeCompleteCallback_, shared_from_this()));
      }
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG_ERROR("TcpConnection::sendInLoop");
        if (errno == EPIPE || errno == ECONNRESET) {
          faultError = true;
        }
      }
    }
  }

  if (!faultError && remaining > 0) {
    std::size_t oldlen = outputBuffer_.readableBytes();
    if (oldlen + remaining >= highWaterMark_ && oldlen < highWaterMark_ &&
        highWaterMarkCallback_) {
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(),
                                   oldlen + remaining));
    }
    outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdown() {
  if (state_ == kConnected) {
    setState(kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop() {
  if (!channel_->isWriting())  // 说明当前outputBuffer_的数据全部向外发送完成
  {
    socket_->shutdownWrite();
  }
}

// 连接建立
void TcpConnection::connectEstablished() {
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading();

  // 新连接建立 执行回调
  connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed() {
  if (state_ == kConnected) {
    setState(kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());
  }
  channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {
  loop_->assertInLoopThread();
  int savedErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0) {
    LOG_DEBUG(
        "messageCallback_(shared_from_this(), &inputBuffer_, receiveTime) -> "
        "%zu",
        n);
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  } else if (n == 0) {
    handleClose();
  } else {
    errno = savedErrno;
    LOG_ERROR("TcpConnection::handleRead");
    handleError();
  }
}

void TcpConnection::handleWrite() {
  if (channel_->isWriting()) {
    int savedErrno = 0;
    ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
    if (n > 0) {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0) {
        channel_->disableWriting();
        if (writeCompleteCallback_) {
          loop_->queueInLoop(
              std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting) {
          shutdownInLoop();
        }
      }
    } else {
      LOG_ERROR("TcpConnection::handleWrite");
    }
  } else {
    LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->fd());
  }
}

void TcpConnection::handleClose() {
  LOG_INFO("TcpConnection::handleClose fd=%d state=%d\n", channel_->fd(),
           (int)state_);
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr connPtr(shared_from_this());
  connectionCallback_(connPtr);  // 执行连接关闭的回调
  closeCallback_(connPtr);
}

void TcpConnection::handleError() {
  int optval;
  socklen_t optlen = sizeof optval;
  int err = 0;
  if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) <
      0) {
    err = errno;
  } else {
    err = optval;
  }
  LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(),
            err);
}

}  // namespace Tnet
