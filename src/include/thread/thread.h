#pragma once

#include <unistd.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "util/macros.h"

namespace Tnet {

class Thread {
 public:
  using ThreadFunc = std::function<void()>;

  explicit Thread(ThreadFunc, const std::string &name = std::string());
  ~Thread();

  DISALLOW_COPY(Thread)

  void start();
  void join();

  bool started() { return started_; }
  pid_t tid() const { return tid_; }
  const std::string &name() const { return name_; }

  static int numCreated() { return numCreated_; }

 private:
  void setDefaultName();

  bool started_;
  bool joined_;
  std::shared_ptr<std::thread> thread_;
  pid_t tid_;
  ThreadFunc func_;  // 线程回调函数
  std::string name_;
  static std::atomic_int numCreated_;
};
}  // namespace Tnet
