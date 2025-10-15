#ifndef LOAD_BALANCER_H_
#define LOAD_BALANCER_H_
#include <server.h>

typedef struct LoadBalancer
{
  Server *server;
  struct addrinfo *res;
} LoadBalancer;

void lb_shutdown(LoadBalancer *lb);
LoadBalancer *load_balancer_init();
char *handle_client(int client_fd);

#endif LOAD_BALANCER_H_