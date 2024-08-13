#include "protobuf/RpcServer.h"

#include "protobuf/RpcChannel.h"
#include "util/log.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

namespace Tnet {

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

RpcServer::RpcServer(EventLoop* loop, const InetAddress& listenAddr)
    : server_(loop, listenAddr, "RpcServer") {
  server_.setConnectionCallback(std::bind(&RpcServer::onConnection, this, _1));
}

void RpcServer::registerService(google::protobuf::Service* service) {
  const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
  services_[desc->full_name()] = service;
}

void RpcServer::start() {
  server_.start();
}

void RpcServer::onConnection(const TcpConnectionPtr& conn) {
  LOG_INFO("RpcServer - %s -> %s is %s", conn->peerAddress().toIpPort().c_str(),
           conn->localAddress().toIpPort().c_str(),
           (conn->connected() ? "UP" : "DOWN"));

  if (conn->connected()) {
    RpcChannelPtr channel = std::make_shared<RpcChannel>(conn);
    channel->setServices(&services_);
    conn->setMessageCallback(
        std::bind(&RpcChannel::onMessage, channel.get(), _1, _2, _3));
    conn->setContext(channel);
  } else {
    conn->setContext(RpcChannelPtr());
  }
}
}  // namespace Tnet
