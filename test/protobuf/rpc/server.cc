#include "sudoku.pb.h"

#include "event/eventloop.h"
#include "protobuf/RpcServer.h"
#include "util/log.h"

#include <unistd.h>

namespace sudoku {

class SudokuServiceImpl : public SudokuService {
  public:
  virtual void Solve(::google::protobuf::RpcController* controller,
                     const ::sudoku::SudokuRequest* request,
                     ::sudoku::SudokuResponse* response,
                     ::google::protobuf::Closure* done) {
    response->set_solved(true);
    response->set_checkerboard("123456");
    LOG_DEBUG("request -> %s",request->DebugString().c_str());
    LOG_DEBUG("response -> %s",response->DebugString().c_str());
    done->Run();
  }
};

}  // namespace sudoku

int main() {
  Tnet::Logger log;
  log.init();
  LOG_INFO("pid = %d", getpid());
  Tnet::EventLoop loop;
  Tnet::InetAddress listenAddr(9981);
  sudoku::SudokuServiceImpl impl;
  Tnet::RpcServer server(&loop, listenAddr);
  server.registerService(&impl);
  server.start();
  loop.loop();
  google::protobuf::ShutdownProtobufLibrary();
}
