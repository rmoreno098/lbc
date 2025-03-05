#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DOMAIN "127.0.0.1"
#define PORT "48963"
#define BACKLOG 5
#define DEFAULT_BUFFER_SIZE 9216

struct addrinfo hints, *res;

void parse_request(int client_fd) {
  char request[DEFAULT_BUFFER_SIZE];

  int bytes_recv = recv(client_fd, request, DEFAULT_BUFFER_SIZE, 0);
  if (bytes_recv == DEFAULT_BUFFER_SIZE) {
    perror("Buffer full, increase buffer size\n");
    exit(1);
  }

  printf("Size of request: %d\n%s", bytes_recv, request);
}

int main() {
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo(DOMAIN, PORT, &hints, &res);
  if (status != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sock_fd == -1) {
    perror("socket error");
    exit(1);
  }

  if (bind(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
    perror("bind error");
    close(sock_fd);
    exit(1);
  }

  if (listen(sock_fd, BACKLOG) < 0) {
    perror("listen error");
    close(sock_fd);
    exit(1);
  }

  printf("Server listening on %s:%s...\n", DOMAIN, PORT);

  int client_fd;
  struct sockaddr_storage client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  while (1) {
    client_fd =
        accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    parse_request(client_fd);

    char *response =
        "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><h1>Hello, "
        "World!</h1></body></html>";
    int x = send(client_fd, response, strlen(response), 0);
    if (x != strlen(response)) {
      printf("Data send did not equal | actual: %d | expected: %lu\n", x,
             strlen(response));
    }

    close(client_fd);
  }
  freeaddrinfo(res);

  return 0;
}
