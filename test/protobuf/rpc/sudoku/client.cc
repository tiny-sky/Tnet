#include "sudoku.pb.h"

#include "event/eventloop.h"
#include "protobuf/RpcChannel.h"
#include "tcpserver/inetaddress.h"
#include "tcpserver/tcpclient.h"
#include "tcpserver/tcpconnection.h"
#include "util/log.h"
#include "event/callbacks.h"

#include <google/protobuf/message.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

using namespace Tnet;

class RpcClient {
  public:
  RpcClient(EventLoop* loop, const InetAddress& serverAddr)
      : loop_(loop),
        client_(loop, serverAddr, "RpcClient"),
        channel_(new RpcChannel),
        stub_(channel_.get()) {
    client_.setConnectionCallback(
        std::bind(&RpcClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&RpcChannel::onMessage, channel_.get(), _1, _2, _3));
  }

  void connect() { client_.connect(); }

  private:
  void onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
      channel_->setConnection(conn);
      sudoku::SudokuRequest request;
      request.set_checkerboard("001010");
      sudoku::SudokuResponse* response = new sudoku::SudokuResponse;

      LOG_DEBUG("request -> %s",request.DebugString().c_str());

      stub_.Solve(nullptr, &request, response,
                  NewCallback(this, &RpcClient::solved, response));
    }
  }

  void solved(sudoku::SudokuResponse* resp) {
    LOG_INFO("%s", resp->DebugString().c_str());
    client_.disconnect();
  }
  EventLoop* loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  sudoku::SudokuService::Stub stub_;
};

int main(int argc, char* argv[]) {
  LOG_INFO("pid = %d", getpid());
  if (argc > 1) {
    EventLoop loop;
    InetAddress serverAddr(9981, argv[1]);

    RpcClient rpcClient(&loop, serverAddr);
    rpcClient.connect();
    loop.loop();
  } else {
    std::cout << "Usage: " << argv[0] << "host_ip" << std::endl;
  }

  google::protobuf::ShutdownProtobufLibrary();
}
