#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DOMAIN "127.0.0.1"
#define PORT "48963"
#define BACKLOG 5

struct addrinfo hints, *res;

int main()
{
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int status;
  if ((status = getaddrinfo(DOMAIN, PORT, &hints, &res)) != 0)
  {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  int sockfd;
  if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) ==
      -1)
  {
    fprintf(stderr, "socket error: %s\n", gai_strerror(sockfd));
    exit(1);
  }

  if ((status = bind(sockfd, res->ai_addr, res->ai_addrlen)) == -1)
  {
    fprintf(stderr, "bind error: %s\n", gai_strerror(status));
    exit(1);
  }

  if ((status = listen(sockfd, 5)) == -1)
  {
    fprintf(stderr, "listen error: %s\n", gai_strerror(status));
    exit(1);
  }

  int client_fd;
  socklen_t client_addr;
  while (1)
  {
    if ((client_fd = accept(sockfd, (struct sockaddr *)&res, &client_addr)) == -1)
    {
      fprintf(stderr, "listen accept: %s\n", gai_strerror(status));
      exit(1);
    }

    printf("Connection made! %d\n", client_fd);
  }

  freeaddrinfo(res);

  return 0;
}
