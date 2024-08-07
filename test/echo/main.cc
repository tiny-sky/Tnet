#include "echo.h"

#include "util/log.h"
#include "event/eventloop.h"

#include "unistd.h"

// g++ -o echo echo.cc main.cc -lTnet -lpthread -I../../src/include -fsanitize=address -lfmt

int main() {
    Tnet::Logger log;
    log.init();
    Tnet::EventLoop loop;
    Tnet::InetAddress listenAddr(2007);
    EchoServer server(&loop, listenAddr);
    server.start();
    loop.loop();
}