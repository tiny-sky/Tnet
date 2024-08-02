#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "util/macros.h"

namespace Tnet {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
  ~EventLoopThreadPool();

  DISALLOW_COPY(EventLoopThreadPool)

  void setThreadNum(int numThreads) { numThreads_ = numThreads; }

  void start(const ThreadInitCallback &cb = ThreadInitCallback());

  // baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
  EventLoop *getNextLoop();

  std::vector<EventLoop *> getAllLoops();

  bool started() const { return started_; }
  const std::string name() const { return name_; }

 private:
  EventLoop *baseLoop_;
  std::string name_;
  bool started_;
  int numThreads_;
  int next_;  // 轮询的下标
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop *> loops_;
};
}  // namespace Tnet
