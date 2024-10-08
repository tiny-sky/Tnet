#include "echo.pb.h"

#include "event/callbacks.h"
#include "event/eventloop.h"
#include "protobuf/RpcChannel.h"
#include "tcpserver/inetaddress.h"
#include "tcpserver/tcpclient.h"
#include "tcpserver/tcpconnection.h"
#include "util/log.h"

#include <google/protobuf/message.h>

using namespace Tnet;

class EchoClient {
  public:
  EchoClient(EventLoop* loop, const InetAddress& serverAddr)
      : loop_(loop),
        client_(loop, serverAddr, "RpcClient"),
        channel_(new RpcChannel),
        stub_(channel_.get()) {
    client_.setConnectionCallback(
        std::bind(&EchoClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&RpcChannel::onMessage, channel_.get(), _1, _2, _3));
  }

  void connect() { client_.connect(); }

  private:
  void onConnection(const TcpConnectionPtr& conn) {
    LOG_INFO("Connection established");
    if (conn->connected()) {
      channel_->setConnection(conn);
      echo::EchoRequest request;
      request.set_msg("Hello World");
      echo::EchoResponse* response = new echo::EchoResponse;

      stub_.Echo(nullptr, &request, response,
                 NewCallback(this, &EchoClient::echo, response));
    }
  }

  void echo(echo::EchoResponse* resp) {
    std::cout << "Client receive -> " << resp->DebugString().c_str() << std::endl;
    client_.disconnect();
    LOG_DEBUG("Disconnect...");
  }
  EventLoop* loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  echo::EchoService_Stub stub_;
};

int main(int argc, char* argv[]) {
  if (argc > 1) {
    Tnet::Logger log;
    log.init();
    EventLoop loop;
    InetAddress serverAddr(6666, argv[1]);

    EchoClient client(&loop, serverAddr);
    client.connect(); // 建立 sockfd ，监听可写事件
    loop.loop();
  } else {
    std::cout << "Usage: " << argv[0] << "host_ip" << std::endl;
  }

  google::protobuf::ShutdownProtobufLibrary();
}