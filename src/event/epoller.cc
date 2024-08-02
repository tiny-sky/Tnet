#include "epoller.h"

#include <assert.h>

#include "event/channel.h"
#include "util/log.h"

namespace Tnet {

const int kNew = -1;  // 某个channel还没添加至Poller
                      // channel的成员index_初始化为-1

const int kAdded = 1;    // 某个channel已经添加至Poller
const int kDeleted = 2;  // 某个channel已经从Poller删除

Epoller::Epoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
  if (epollfd_ < 0) {
    LOG_ERROR("epoll_create error:%d \n", errno);
  }
}

Epoller::~Epoller() {
  ::close(epollfd_);
}

Timestamp Epoller::poll(int timeoutMs, ChannelList* activeChannels) {
  LOG_DEBUG("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

  int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
                               static_cast<int>(events_.size()), timeoutMs);
  int saveErrno = errno;
  Timestamp now;

  if (numEvents > 0) {
    LOG_DEBUG("%d events happend\n", numEvents);
    fillActiveChannels(numEvents, activeChannels);
    if (static_cast<std::size_t>(numEvents) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (numEvents == 0) {
    LOG_DEBUG("%s timeout!\n", __FUNCTION__);
  } else {
    if (saveErrno != EINTR) {
      errno = saveErrno;
      LOG_ERROR("EPollPoller::poll() error!");
    }
  }
  return now;
}

void Epoller::updateChannel(Channel* channel) {
  const int index = channel->index();
  LOG_INFO("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel->fd(),
           channel->events(), index);

  if (index == kNew || index == kDeleted) {
    // a new one, add with EPOLL_CTL_ADD
    int fd = channel->fd();
    if (index == kNew) {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    } else {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }

    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);
  } else {
    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());

    if (channel->isNoneEvent()) {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    } else {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void Epoller::removeChannel(Channel* channel) {
  int fd = channel->fd();
  channels_.erase(fd);

  LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

  int index = channel->index();
  if (index == kAdded) {
    update(EPOLL_CTL_DEL, channel);
  }

  // kNew kDeleted ?
  channel->set_index(kNew);
}

void Epoller::fillActiveChannels(int numEvents,
                                 ChannelList* activeChannels) const {
  for (int i = 0; i < numEvents; ++i) {
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
    channel->set_revents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void Epoller::update(int operation, Channel* channel) {
  epoll_event event;
  ::memset(&event, 0, sizeof(event));

  int fd = channel->fd();

  event.events = channel->events();
  event.data.fd = fd;
  event.data.ptr = channel;

  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      LOG_ERROR("epoll_ctl del error:%d\n", errno);
    } else {
      LOG_ERROR("epoll_ctl add/mod error:%d\n", errno);
    }
  }
}

Poller* newDefaultPoller(EventLoop* loop) {
  return new Epoller(loop);
}

}  // namespace Tnet
