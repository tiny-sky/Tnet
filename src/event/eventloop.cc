#include "event/eventloop.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <memory>

#include "event/channel.h"
#include "event/poller.h"
#include "util/log.h"

namespace Tnet {

// 防止一个线程创建多个EventLoop
__thread EventLoop* t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;  // 10000毫秒 = 10秒钟

int createEventfd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    LOG_ERROR("eventfd error:%d\n", errno);
  }
  return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      threadId_(CurrentThread::tid()),
      poller_(newDefaultPoller(this)),
      callingPendingFunctors_(false),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {
  LOG_INFO("Create EventLoop in thread -> [%d]\n", threadId_);
  if (t_loopInThisThread) {
    LOG_ERROR("Another EventLoop %p exists in this thread %d\n",
              t_loopInThisThread, threadId_);
  } else {
    t_loopInThisThread = this;
  }

  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  t_loopInThisThread = nullptr;
}

void EventLoop::abortNotInLoopThread() {
  LOG_ERROR(
      "EventLoop::abortNotInLoopThread was created in threadId_ %d ,current "
      "thread id =  %d",
      threadId_, CurrentThread::tid());
}

void EventLoop::loop() {
  looping_.store(true, std::memory_order_release);
  quit_.store(false, std::memory_order_release);

  LOG_INFO("EventLoop %p start looping\n", this);

  while (!quit_) {
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

    for (auto* channel : activeChannels_) {
      channel->handleEvent(pollReturnTime_);
    }

    doPendingFunctors();
  }
  LOG_INFO("EventLoop %p stop looping.\n", this);
  looping_.store(false, std::memory_order_release);
}

void EventLoop::quit() {
  quit_.store(true, std::memory_order_release);

  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = write(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
  }
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_.store(true, std::memory_order_release);

  {
    std::unique_lock<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const Functor& functor : functors) {
    functor();  // 执行当前loop需要执行的回调操作
  }

  callingPendingFunctors_.store(false, std::memory_order_release);
}

void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(cb);
  }

  if (!isInLoopThread() || callingPendingFunctors_) {
    wakeup();
  }
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
  }
}

void EventLoop::updateChannel(Channel* channel) {
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
  return poller_->hasChannel(channel);
}

}  // namespace Tnet
