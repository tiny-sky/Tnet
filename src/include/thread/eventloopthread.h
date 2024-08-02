#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

#include "util/macros.h"
#include "thread/thread.h"

namespace Tnet {

class EventLoop;

class EventLoopThread {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                  const std::string &name = std::string());
  ~EventLoopThread();

  DISALLOW_COPY(EventLoopThread)

  EventLoop *startLoop();

 private:
  void threadFunc();

  EventLoop *loop_;
  bool exiting_;
  Thread thread_;
  std::mutex mutex_;              // 互斥锁
  std::condition_variable cond_;  // 条件变量
  ThreadInitCallback callback_;
};

}  // namespace Tnet
