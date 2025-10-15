#ifndef SERVER_H_
#define SERVER_H_

typedef struct Server {
  char *domain;
  char *port;
  int sock_fd;
} Server;

int server_connect(char *domain, char *port);
void server_disconnect(Server *server);

#endif SERVER_H_