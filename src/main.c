#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <server.h>
#include <load_balancer.h>

#define BACKLOG 5
#define DEFAULT_BUFFER_SIZE 4096

Server servers[] = {{"127.0.0.1", "12345"}, {"127.0.0.1", "12346"}};
volatile sig_atomic_t stop_server = 0;

static void signal_handler(int signum)
{
  if (signum == SIGINT)
  {
    stop_server = 1;
  }
}

void set_non_block(int fd)
{
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main()
{
  LoadBalancer *lb = load_balancer_init();
  struct epoll_event events[BACKLOG];

  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGINT, &sa, NULL) == -1)
  {
    perror("SIGINT");
    return -1;
  }

  int epoll_fd = epoll_create(1);
  if (epoll_fd <= 0)
  {
    perror("epoll_create");
    exit(1);
  }

  set_non_block(lb->server->sock_fd);
  epoll_ctl_add(epoll_fd, lb->server->sock_fd);

  while (!stop_server)
  {
    int nfds = epoll_wait(epoll_fd, events, BACKLOG, -1);
    if (nfds < 0)
    {
      if (errno == EINTR && stop_server)
        break;
      perror("epoll_wait");
      continue;
    }

    for (int i = 0; i < nfds; i++)
    {
      int fd = events[i].data.fd;

      if (fd == lb->server->sock_fd)
      { // accept new connections
        int client_fd;
        struct sockaddr_storage client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        client_fd = accept(lb->server->sock_fd, (struct sockaddr *)&client_addr,
                           &client_addr_len);

        if (client_fd < 0)
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
          perror("accept");
          continue;
        }

        set_non_block(client_fd);
        epoll_ctl_add(epoll_fd, client_fd);
      }
      else
      { // handle current connections
        char *client_request = handle_client(fd);
        int backend_socket =
            server_connect(servers[0].domain,
                           servers[0].port); // connects to a backend server

        // forward client payload to server
        int client_send =
            send(backend_socket, client_request, strlen(client_request),
                 0); // Track number of bytes sent
        if (client_send < 0)
        {
          perror("send error");
          close(backend_socket);
          exit(1);
        }

        // send server response back to client
        char request[DEFAULT_BUFFER_SIZE];
        int bytes_recv = recv(backend_socket, request, DEFAULT_BUFFER_SIZE, 0);
        if (bytes_recv < 0)
        {
          perror("recv error");
          close(backend_socket);
          exit(1);
        }
        close(backend_socket);

        int client_response = send(fd, request, strlen(request), 0);
        close(fd);
        printf("Received %d bytes from backend server and sent %d bytes to "
               "client\n",
               bytes_recv, client_response);
      }
    } // nfds
  } // stop_server

  lb_shutdown(lb);
  close(epoll_fd);

  return 0;
}

void epoll_ctl_add(int epfd, int fd)
{
  struct epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = fd;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) < 0)
  {
    perror("epoll_ctl");
    exit(1);
  }
}
