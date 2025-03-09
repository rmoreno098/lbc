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
#define DEFAULT_BUFFER_SIZE 4096

typedef struct Server {
  char *port;
  char *domain;
  int sock_fd;
} Server;

typedef struct LoadBalancer {
  Server *server;
  struct addrinfo *res;
} LoadBalancer;

Server servers[] = {{"127.0.0.1", "12345"}, {"127.0.0.1", "12346"}};

LoadBalancer *load_balancer_init();
void server_connect(char *domain, char *port);
void server_disconnect(Server *server);
void handle_client(int client_fd);

int main() {
  LoadBalancer *lb = load_balancer_init();

  int client_fd;
  struct sockaddr_storage client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  while (1) {
    client_fd = accept(lb->server->sock_fd, (struct sockaddr *)&client_addr,
                       &client_addr_len);
    handle_client(client_fd);
  }

  server_disconnect(lb->server);
  freeaddrinfo(lb->res);

  return 0;
}

void server_connect(char *domain, char *port) {}

void server_disconnect(Server *server) {
  close(server->sock_fd);
  printf("Server shutdown\n");
  free(server);
}

void handle_client(int client_fd) {
  char request[DEFAULT_BUFFER_SIZE]; // Make dynamic

  // Handle this better with a loop
  int bytes_recv = recv(client_fd, request, DEFAULT_BUFFER_SIZE, 0);
  if (bytes_recv < 0) {
    perror("recv error");
    close(client_fd);
    exit(1);
  }

  printf("Size of request: %d\n%s", bytes_recv, request);
  char *method = strtok(request, " ");
  char *path = strtok(NULL, " ");
  char *protocol = strtok(NULL, "\r\n");

  if (strcmp(protocol, "HTTP/1.1") != 0) {
    char *response = "HTTP/1.1 400 Bad Request\nContent-Type: text/html\n\n";
    send(client_fd, response, strlen(response), 0);
    close(client_fd);
    exit(1);
  }

  if (strcmp(method, "GET") == 0) {
    // handle_get(client_fd, path);
  } else if (strcmp(method, "POST") == 0) {
    // handle_post(client_fd, path);
  } else {
    char *response = "HTTP/1.1 405 Method Not Allowed\nContent-Type: "
                     "text/html\n\n<html><body><h1>405 Method Not "
                     "Allowed</h1></body></html>";
    send(client_fd, response, strlen(response), 0);
    close(client_fd);
    exit(1);
  }

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

LoadBalancer *load_balancer_init() {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));

  LoadBalancer *lb = malloc(sizeof(struct LoadBalancer));
  lb->server = malloc(sizeof(struct Server));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo(DOMAIN, PORT, &hints, &lb->res);
  if (status != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  int sock_fd =
      socket(lb->res->ai_family, lb->res->ai_socktype, lb->res->ai_protocol);
  if (sock_fd == -1) {
    perror("socket error");
    exit(1);
  }

  if (bind(sock_fd, lb->res->ai_addr, lb->res->ai_addrlen) < 0) {
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
  lb->server->sock_fd = sock_fd;

  return lb;
}
