// Copyright 2025 RocketFS

#include "datanode/service/rpc_server.h"

#include <arpa/inet.h>
#include <ff_api.h>
#include <ff_epoll.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <cstring>

#include "common/logger.h"

namespace rocketfs {

constexpr int kMaxEvents = 1024;

char html[] =
    "HTTP/1.1 200 OK\r\n"
    "Server: F-Stack\r\n"
    "Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 438\r\n"
    "Last-Modified: Tue, 21 Feb 2017 09:44:03 GMT\r\n"
    "Connection: keep-alive\r\n"
    "Accept-Ranges: bytes\r\n"
    "\r\n"
    "<!DOCTYPE html>\r\n"
    "<html>\r\n"
    "<head>\r\n"
    "<title>Welcome to F-Stack!</title>\r\n"
    "<style>\r\n"
    "    body {  \r\n"
    "        width: 35em;\r\n"
    "        margin: 0 auto; \r\n"
    "        font-family: Tahoma, Verdana, Arial, sans-serif;\r\n"
    "    }\r\n"
    "</style>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Welcome to F-Stack!</h1>\r\n"
    "\r\n"
    "<p>For online documentation and support please refer to\r\n"
    "<a href=\"http://F-Stack.org/\">F-Stack.org</a>.<br/>\r\n"
    "\r\n"
    "<p><em>Thank you for using F-Stack.</em></p>\r\n"
    "</body>\r\n"
    "</html>";

RpcServer::RpcServer(int listening_port) {
  ff_init(0, nullptr);
  sock_fd_ = ff_socket(AF_INET, SOCK_STREAM, 0);
  CHECK_GE(sock_fd_, 0);
  int on = 1;
  CHECK_EQ(ff_ioctl(sock_fd_, FIONBIO, &on), 0);

  struct sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(listening_port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  CHECK_EQ(ff_bind(sock_fd_,
                   reinterpret_cast<struct linux_sockaddr*>(&addr),
                   sizeof(addr)),
           0);
  CHECK_EQ(ff_listen(sock_fd_, kMaxEvents), 0);

  epoll_fd_ = ff_epoll_create(0);
  CHECK_GT(epoll_fd_, 0);
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = sock_fd_;
  CHECK_EQ(ff_epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sock_fd_, &ev), 0);

  ff_run(
      [](void* arg) {
        auto server = static_cast<RpcServer*>(arg);
        server->Run();
        return 0;
      },
      this);
}

void RpcServer::Run() {
  struct epoll_event events[kMaxEvents];
  int nevents = ff_epoll_wait(epoll_fd_, events, kMaxEvents, -1);
  for (auto i = 0; i < nevents; i++) {
    // Handle new connection.
    if (events[i].data.fd == sock_fd_) {
      int nclientfd = ff_accept(sock_fd_, nullptr, nullptr);
      CHECK_GE(nclientfd, 0);
      struct epoll_event ev;
      ev.events = EPOLLIN;
      ev.data.fd = nclientfd;
      CHECK_EQ(ff_epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, nclientfd, &ev), 0);
    }
    // Handle client request.
    else {
      if (events[i].events & EPOLLERR) {
        // Simply close socket.
        ff_epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, events[i].data.fd, NULL);
        ff_close(events[i].data.fd);
      } else if (events[i].events & EPOLLIN) {
        char buf[256];
        size_t readlen = ff_read(events[i].data.fd, buf, sizeof(buf));
        if (readlen > 0) {
          ff_write(events[i].data.fd, html, sizeof(html) - 1);
        } else {
          ff_epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, events[i].data.fd, NULL);
          ff_close(events[i].data.fd);
        }
      }
    }
  }
}

}  // namespace rocketfs
