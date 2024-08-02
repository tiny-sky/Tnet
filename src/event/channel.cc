#include "channel.h"

#include <assert.h>

#include <sys/epoll.h>

#include "event/eventloop.h"
#include "util/log.h"

namespace Tnet {

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false) {}

Channel::~Channel() {
  if (loop_->isInLoopThread()) {
    assert(!loop_->hasChannel(this));
  }
}

void Channel::tie(const std::shared_ptr<void>& obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::update() {
  loop_->updateChannel(this);
}

void Channel::remove() {
  loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime) {
  if (tied_) {
    std::shared_ptr<void> guard = tie_.lock();
    if (guard) {
      handleEventWithGuard(receiveTime);
    }
  } else {
    handleEventWithGuard(receiveTime);
  }
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
  LOG_INFO("channel handleEvent revents:%d\n", revents_);

  // shutdown 触发 EPOLLHUP 且无写数据
  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
    if (closeCallback_) {
      closeCallback_();
    }
  }

  // 触发错误
  if (revents_ & EPOLLERR) {
    if (errorCallback_) {
      errorCallback_();
    }
  }

  // 读事件
  if (revents_ & (EPOLLIN | EPOLLPRI)) {
    if (readCallback_) {
      readCallback_(receiveTime);
    }
  }
  // 写事件
  if (revents_ & EPOLLOUT) {
    if (writeCallback_) {
      writeCallback_();
    }
  }
}
}  // namespace Tnet
