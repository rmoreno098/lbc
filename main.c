#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define DOMAIN "127.0.0.1"
#define PORT "48963"
#define BACKLOG 5
#define DEFAULT_BUFFER_SIZE 4096

volatile sig_atomic_t stop_server = 0;

typedef struct Server {
  char *domain;
  char *port;
  int sock_fd;
} Server;

typedef struct LoadBalancer {
  Server *server;
  struct addrinfo *res;
} LoadBalancer;

Server servers[] = {{"127.0.0.1", "12345"}, {"127.0.0.1", "12346"}};

LoadBalancer *load_balancer_init();
char *handle_client(int client_fd, struct sockaddr_storage client_addr);
void server_disconnect(Server *server);
int server_connect(char *domain, char *port);
void epoll_ctl_add(int epfd, int fd);

static void signal_handler(int signum) {
  if (signum == SIGINT) {
    stop_server = 1;
  }
}

int main() {
  LoadBalancer *lb = load_balancer_init();

  int client_fd;
  struct sockaddr_storage client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("SIGINT");
    return -1;
  }

  while (!stop_server) {
    client_fd = accept(lb->server->sock_fd, (struct sockaddr *)&client_addr,
                       &client_addr_len);
    if (client_fd < 0) {
      if (errno == EINTR && stop_server)
        break;
      perror("accept");
      continue;
    }

    int epoll_fd = epoll_create(1);
    if (epoll_fd <= 0) {
      perror("epoll_create");
      exit(1);
    }
    epoll_ctl_add(epoll_fd, lb->server->sock_fd);

    char *client_request = handle_client(client_fd, client_addr);
    int backend_socket = server_connect(servers[0].domain, servers[0].port);
    int client_send =
        send(backend_socket, client_request, strlen(client_request),
             0); // Track number of bytes sent
    if (client_send < 0) {
      perror("send error");
      close(backend_socket);
      exit(1);
    }

    char request[DEFAULT_BUFFER_SIZE];
    int bytes_recv = recv(backend_socket, request, DEFAULT_BUFFER_SIZE, 0);
    if (bytes_recv < 0) {
      perror("recv error");
      close(backend_socket);
      exit(1);
    }
    close(backend_socket);

    int client_response = send(client_fd, request, strlen(request), 0);
    close(client_fd);
    printf(
        "Received %d bytes from backend server and sent %d bytes to client\n",
        bytes_recv, client_response);
  }

  printf("\nShutting down...\n");
  server_disconnect(lb->server);
  freeaddrinfo(lb->res);
  printf("Load balancer shutdown.\n");

  return 0;
}

void epoll_ctl_add(int epfd, int fd) {
  struct epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = fd;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) < 0) {
    perror("epoll_ctl");
    exit(1);
  }
}

int server_connect(char *domain, char *port) {
  int sock_fd, conn;
  struct sockaddr_in servaddr;

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(domain);
  servaddr.sin_port = htons(atoi(port));
  printf("Connecting to backend server: %s on port %d\n", domain, atoi(port));

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    perror("socket error");
    exit(1);
  }

  conn = connect(sock_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  if (conn < 0) {
    perror("connect error");
    exit(1);
  }

  printf("Connected to backend server\n");
  return sock_fd;
}

void server_disconnect(Server *server) {
  close(server->sock_fd);
  free(server);
}

char *handle_client(int client_fd, struct sockaddr_storage client_addr) {
  char *request = malloc(DEFAULT_BUFFER_SIZE);
  bzero(request, DEFAULT_BUFFER_SIZE);

  // Handle this better with a loop
  int bytes_recv = recv(client_fd, request, DEFAULT_BUFFER_SIZE, 0);
  if (bytes_recv < 0) {
    perror("recv error");
    close(client_fd);
    exit(1);
  }
  printf("Size of request: %d\n%s", bytes_recv, request);
  return request;
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

  int fd =
      setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
  if (fd < 0) {
    perror("setsockopt error");
    close(sock_fd);
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

  printf("Load balancer listening on %s:%s...\n", DOMAIN, PORT);
  lb->server->sock_fd = sock_fd;

  return lb;
}
