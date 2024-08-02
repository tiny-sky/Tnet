#include "event/poller.h"
#include "event/channel.h"

namespace Tnet {
Poller::Poller(EventLoop *loop) : ownerLoop_(loop) {}

bool Poller::hasChannel(Channel *channel) const {
  auto it = channels_.find(channel->fd());
  return it != channels_.end() && it->second == channel;
}

}  // namespace Tnet