#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "thread/curthread.h"
#include "util/macros.h"
#include "util/timestamp.h"

namespace Tnet {

class Channel;
class Poller;

// 代表所有I/O事件被处理的事件循环。
class EventLoop {
  public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  void loop();

  void quit();

  Timestamp pollReturnTime() const { return pollReturnTime_; }

  /// 在当前循环中执行一个回调。
  void runInLoop(Functor cb);

  /// 将回调函数加入循环队列并唤醒循环。
  void queueInLoop(Functor cb);

  /// 唤醒循环的线程。
  void wakeup();

  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);
  bool hasChannel(Channel* channel);

  /// 判断EventLoop对象是否在它自己的线程中。
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

  void assertInLoopThread() {
    if (!isInLoopThread()) {
      abortNotInLoopThread();
    }
  }

  private:
  void abortNotInLoopThread();
  void handleRead();         // 处理来自唤醒文件描述符的读事件。
  void doPendingFunctors();  // 执行队列中的回调。

  using ChannelList = std::vector<Channel*>;

  std::atomic<bool> looping_;  // 如果循环当前正在运行，则为真。
  std::atomic<bool> quit_;     // 如果循环应该退出，则为真。

  const pid_t threadId_;  // 创建此EventLoop的线程的ID。

  Timestamp pollReturnTime_;        // 最后一次poll调用返回的时间。
  std::unique_ptr<Poller> poller_;  // 本循环使用的Poller实例。

  int wakeupFd_;  // 用于唤醒循环的文件描述符。
  std::unique_ptr<Channel> wakeupChannel_;  // 唤醒文件描述符的通道。

  ChannelList activeChannels_;  // 有待处理事件的通道列表。

  std::atomic<bool> callingPendingFunctors_;  // 如果有待执行的回调，则为真。
  std::vector<Functor> pendingFunctors_;  // 要执行的队列回调。
  std::mutex mutex_;  // 保护`pendingFunctors`访问的互斥锁。
};

}  // namespace Tnet
