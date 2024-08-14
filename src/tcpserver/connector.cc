#include "tcpserver/connector.h"

#include <netinet/in.h>
#include "event/channel.h"
#include "event/eventloop.h"
#include "tcpserver/socket.h"
#include "util/log.h"

#include <errno.h>

namespace Tnet {

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs) {
}

Connector::~Connector() {
  LOG_DEBUG("Delete Connector");
}

void Connector::start() {
  connect_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::restart() {
  loop_->assertInLoopThread();
  setState(kDisconnected);
  retryDelayMs_ = kInitRetryDelayMs;
  connect_ = true;
  startInLoop();
}

void Connector::startInLoop() {
  loop_->assertInLoopThread();
  assert(state_ == kDisconnected);
  if (connect_) {
    connect();
  } else {
    LOG_DEBUG("do not connect");
  }
}

void Connector::connect() {
  int sockfd =
      ::socket(serverAddr_.family(), SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
               IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_ERROR("Create sockfd Failed!!");
  }
  int ret = ::connect(
      sockfd, reinterpret_cast<const sockaddr*>(serverAddr_.getSockAddr()),
      static_cast<socklen_t>(sizeof(sockaddr_in)));

  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      connecting(sockfd);
      break;

    default:
      LOG_ERROR("Unexpected error in Connector::connect -> %d", savedErrno);
      ::close(sockfd);
      break;
  }
}

void Connector::connecting(int sockfd) {
  setState(kConnecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, sockfd));
  channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
  channel_->setErrorCallback(std::bind(&Connector::handleError, this));
  channel_->enableWriting();
}

void Connector::handleWrite() {
  LOG_INFO("Connector::handleWrite -> %d", state_);

  if (state_ == kConnecting) {
    int sockfd = removeAndResetChannel();
    int err = Socket::getSocketError(sockfd);
    if (err) {
      LOG_ERROR("Connector::handleWrite -> %s", strerror(err));
      retry(sockfd);
    } else if (Socket::isSelfConnect(sockfd)) {
      LOG_ERROR("Connector::handleWrite - Self connect");
      retry(sockfd);
    } else {
      setState(kConnected);
      if (connect_) {
        newConnectionCallback_(sockfd);
      } else {
        ::close(sockfd);
      }
    }
  }
}

void Connector::handleError() {
  LOG_ERROR("Connector::handleError state -> %d", state_);
  if (state_ == kConnecting) {
    int sockfd = removeAndResetChannel();
    int err = Socket::getSocketError(sockfd);
    LOG_ERROR("SO_ERROR  -> %s", strerror(err));
    retry(sockfd);
  }
}

void Connector::stop() {
  connect_ = false;
  loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop() {
  loop_->assertInLoopThread();
  if (state_ == kConnecting) {
    setState(kDisconnected);
    int sockfd = removeAndResetChannel();
    retry(sockfd);
  }
}

int Connector::removeAndResetChannel() {
  channel_->disableAll();
  channel_->remove();
  int sockfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
  return sockfd;
}

void Connector::resetChannel() {
  channel_.reset();
}

void Connector::retry(int sockfd) {
  ::close(sockfd);
  if (connect_) {
    LOG_INFO("Connector::retry - Retry connecting to %s in %d milliseconds.",
             serverAddr_.toIpPort().c_str(), retryDelayMs_);
    sleep(retryDelayMs_);
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
  } else {
    LOG_DEBUG("do not connect");
  }
}

}  // namespace Tnet
