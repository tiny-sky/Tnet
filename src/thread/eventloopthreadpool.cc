#include "thread/eventloopthreadpool.h"

#include <memory>

#include "thread/eventloopthread.h"

namespace Tnet {
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,
                                         const std::string& nameArg)
    : baseLoop_(baseLoop),
      name_(nameArg),
      started_(false),
      numThreads_(0),
      next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
  started_ = true;

  for (int i = 0; i < numThreads_; i++) {
    std::vector<char> buf(name_.size() + 32);
    snprintf(buf.data(), buf.size(), "%s%d", name_.c_str(), i);

    EventLoopThread* t = new EventLoopThread(cb, buf.data());
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    loops_.push_back(t->startLoop());
  }

  if (numThreads_ == 0 && cb) {
    cb(baseLoop_);
  }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
  EventLoop* loop = baseLoop_;

  if (!loops_.empty()) {
    loop = loops_[next_];
    ++next_;
    if (static_cast<std::size_t>(next_) >= loops_.size()) {
      next_ = 0;
    }
  }
  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
  if (loops_.empty()) {
    return std::vector<EventLoop*>(1, baseLoop_);
  } else {
    return loops_;
  }
}
}  // namespace Tnet
