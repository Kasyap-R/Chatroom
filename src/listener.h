#ifndef LISTENER_H
#define LISTENER_H

#include <netdb.h>

typedef struct Listener {
  int port;
  int sockfd;
} Listener;

void BindListener(Listener *listener);

#endif
