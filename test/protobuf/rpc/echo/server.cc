#include "echo.pb.h"

#include "event/eventloop.h"
#include "protobuf/RpcServer.h"
#include "util/log.h"

#include <unistd.h>
#include <iostream>

namespace Echo {

class EchoServerImpl : public echo::EchoService {
  public:
  virtual void Echo(::google::protobuf::RpcController* controller,
                    const ::echo::EchoRequest* request,
                    ::echo::EchoResponse* response,
                    ::google::protobuf::Closure* done) {
    std::cout << " Server Echo -> " <<request->msg() << std::endl;
    std::string msg("Server Echo -> " + request->msg());
    response->set_msg(msg);
    done->Run();
  }
};
}  // namespace Echo

int main() {
  Tnet::Logger log;
  log.init();
  Tnet::EventLoop loop;
  Tnet::InetAddress listenAddr(6666);
  Echo::EchoServerImpl impl;
  Tnet::RpcServer server(&loop, listenAddr);  //TcpServer + RpcServerSet
  server.registerService(&impl);              // 注册事件
  server.start();   // Listening + Add Read event
  loop.loop();  // 启动epoll_wait + 处理事件触发回调
  google::protobuf::ShutdownProtobufLibrary();
}
