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

#define DEFAULT_BUFFER_SIZE 4096
#define BACKLOG 5
#define DOMAIN "127.0.0.1"
#define PORT "48963"

/*
  Close socket used by loadbalancer and any remaining connections
  */
void lb_shutdown(LoadBalancer *lb)
{
  printf("\nShutting down...\n");
  server_disconnect(lb->server);
  freeaddrinfo(lb->res);
  printf("Load balancer shutdown.\n");
}

/*
  Helper function to startup load balancer
*/
LoadBalancer *load_balancer_init()
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));

  LoadBalancer *lb = malloc(sizeof(struct LoadBalancer));
  lb->server = malloc(sizeof(struct Server));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo(DOMAIN, PORT, &hints, &lb->res);
  if (status != 0)
  {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  int sock_fd =
      socket(lb->res->ai_family, lb->res->ai_socktype, lb->res->ai_protocol);
  if (sock_fd == -1)
  {
    perror("socket error");
    exit(1);
  }

  int fd =
      setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
  if (fd < 0)
  {
    perror("setsockopt error");
    close(sock_fd);
    exit(1);
  }

  if (bind(sock_fd, lb->res->ai_addr, lb->res->ai_addrlen) < 0)
  {
    perror("bind error");
    close(sock_fd);
    exit(1);
  }

  if (listen(sock_fd, BACKLOG) < 0)
  {
    perror("listen error");
    close(sock_fd);
    exit(1);
  }

  printf("Load balancer listening on %s:%s...\n", DOMAIN, PORT);
  lb->server->sock_fd = sock_fd;

  return lb;
}

/*
  Read client's request and determine destination
  (currently reads request in memory, needs to stream)
*/
char *handle_client(int client_fd)
{
  char *request = malloc(DEFAULT_BUFFER_SIZE);
  bzero(request, DEFAULT_BUFFER_SIZE);

  int total_bytes = 0;
  while (1)
  {
    int bytes_recv = recv(client_fd, request, DEFAULT_BUFFER_SIZE, 0);
    if (bytes_recv < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
      {
        break;
      }
      else
      {
        perror("recv error");
        close(client_fd);
        exit(1);
      }
    }
    else if (bytes_recv == 0)
    { // no more bytes to read, close connection
      close(client_fd);
      break;
    }
    total_bytes += bytes_recv; // keep reading
  }

  printf("Size of request: %d\n%s\n", total_bytes, request);
  return request;
}
