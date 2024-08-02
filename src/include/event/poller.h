#pragma once

#include <unordered_map>
#include <vector>

#include "util/macros.h"
#include "util/timestamp.h"
#include "event/eventloop.h"

namespace Tnet {

class Channel;
class EventLoop;

class Poller {
  public:
  using ChannelList = std::vector<Channel*>;

  Poller(EventLoop* loop);
  virtual ~Poller() = default;

  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
  virtual void updateChannel(Channel* channel) = 0;
  virtual void removeChannel(Channel* channel) = 0;

  // 判断参数channel是否在当前的Poller当中
  bool hasChannel(Channel* channel) const;

  void assertInLoopThread() const { ownerLoop_->assertInLoopThread(); }

  protected:
  // map的key:sockfd value:sockfd所属的channel通道类型
  using ChannelMap = std::unordered_map<int, Channel*>;
  ChannelMap channels_;

  private:
  EventLoop* ownerLoop_;  // 定义Poller所属的事件循环EventLoop
};

Poller* newDefaultPoller(EventLoop* loop);

}  // namespace Tnet
