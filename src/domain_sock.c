#include "domain_sock.h"
#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int CreateDomainSock() {
  int sock_fd;
  if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("Domain Socket Creation Error\n");
    exit(EXIT_FAILURE);
  }
  return sock_fd;
}

void BindParentDomainSock(int sock_fd, char *path) {
  struct sockaddr_un message_sockaddr;
  memset(&message_sockaddr, 0, sizeof(message_sockaddr));
  message_sockaddr.sun_family = AF_UNIX;
  strncpy(message_sockaddr.sun_path, path,
          sizeof(message_sockaddr.sun_path) - 1);
  message_sockaddr.sun_path[sizeof(message_sockaddr.sun_path) - 1] =
      '\0';     // Ensure null-termination
  unlink(path); // Removes the file if it already exists from
                // previous execution

  if (bind(sock_fd, (struct sockaddr *)&message_sockaddr,
           sizeof(message_sockaddr)) == -1) {
    perror("bind error");
    exit(EXIT_FAILURE);
  }
}

void ListenOnDomainSock(int sock_fd) {
  if (listen(sock_fd, 15) == -1) {
    perror("listen error");
    exit(EXIT_FAILURE);
  }
}
