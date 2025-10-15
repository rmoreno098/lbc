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

int server_connect(char *domain, char *port)
{
  int sock_fd, conn;
  struct sockaddr_in servaddr;

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(domain);
  servaddr.sin_port = htons(atoi(port));
  printf("Connecting to backend server [%s:%d]...\n", domain, atoi(port));

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0)
  {
    perror("socket error");
    exit(1);
  }

  conn = connect(sock_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  if (conn < 0)
  {
    perror("connect error");
    exit(1);
  }

  printf("Connected to backend server\n");
  return sock_fd;
}
/*
  Close the connection to a server given the server stuct
*/
void server_disconnect(Server *server)
{
  close(server->sock_fd);
  free(server);
}
