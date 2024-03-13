#include "listener.h"
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void BindListener(Listener *listener) {
  struct addrinfo socket_settings;
  struct addrinfo *server_addresses;
  char port_string[6];
  int status;

  memset(&socket_settings, 0, sizeof(socket_settings));
  socket_settings.ai_family = AF_UNSPEC; // AF_UNSPEC means both IPv4 and IPv6
  socket_settings.ai_socktype = SOCK_STREAM;
  socket_settings.ai_flags =
      AI_PASSIVE; // Allows socket to listen on any interface of this PC

  snprintf(port_string, sizeof(port_string), "%d", listener->port);

  // after this server_addresses will point to a list of addrinfo objects
  if ((status = (getaddrinfo(NULL, port_string, &socket_settings,
                             &server_addresses))) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(2);
  }

  struct addrinfo *address_iterator;

  for (address_iterator = server_addresses; address_iterator != NULL;
       address_iterator = address_iterator->ai_next) {
    listener->sockfd =
        socket(address_iterator->ai_family, address_iterator->ai_socktype,
               address_iterator->ai_protocol);
    if (listener->sockfd == -1)
      continue; // Socket Creation Failed

    if (bind(listener->sockfd, address_iterator->ai_addr,
             address_iterator->ai_addrlen) == 0) {
      break; // Successfully binded a socket
    }

    close(listener->sockfd); // This socket failed, try the next
  }

  if (address_iterator == NULL) {
    printf("Failed to bind\n");
    exit(1);
  }

  freeaddrinfo(server_addresses);
}
