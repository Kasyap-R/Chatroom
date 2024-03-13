#include "domain_sock.h"
#include "listener.h"
#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <errno.h>
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

#define MAX_QUEUED_CONNECTIONS 10

int main() {
  typedef struct UserMessage {
    int user_id;
    char message[512];
  } UserMessage;

  Listener listener_instance;
  Listener *listener = &listener_instance;
  listener->port = 2070;

  BindListener(listener);

  if (listen(listener->sockfd, MAX_QUEUED_CONNECTIONS)) {
    perror("listener listen error");
    exit(EXIT_FAILURE);
  }

  int client_fds[10];
  int curr_client_index = 0;

  // Socket to track chat messages from child processes
  char *message_socket_path = "ipc_socket";
  int message_sockfd;

  message_sockfd = CreateDomainSock();
  BindParentDomainSock(message_sockfd, message_socket_path);
  ListenOnDomainSock(message_sockfd);

  // Creating a set of sockets
  int max_fd;
  struct timeval timeout;

  fd_set read_fds;

  max_fd =
      (listener->sockfd > message_sockfd ? listener->sockfd : message_sockfd);

  timeout.tv_sec = 2;
  timeout.tv_usec = 0;

  while (1) {
    FD_ZERO(&read_fds);
    FD_SET(listener->sockfd, &read_fds);
    FD_SET(message_sockfd, &read_fds);

    int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (activity == 0) {
      continue;
    }
    if (FD_ISSET(listener->sockfd, &read_fds)) {
      printf("Connection Attempt Made\n");
      fflush(stdout);
      struct sockaddr_in client_addr;
      socklen_t client_len = sizeof(client_addr);

      int client_sockfd = accept(listener->sockfd,
                                 (struct sockaddr *)&client_addr, &client_len);

      if (client_sockfd < 0) {
        printf("ERROR accepting connection");
        return 1;
      }

      if (curr_client_index >= sizeof(client_fds) / sizeof(int)) {
        break;
      }

      client_fds[curr_client_index] = client_sockfd;

      char *accept_message = "Welcome to the Chatroom\n";
      if (send(client_sockfd, accept_message, strlen(accept_message), 0) ==
          -1) {
        printf("ERROR sending welcome message");
        return 1;
      }
      int pid = fork();
      while (pid == 0) { // While in child process
        fflush(stdout);
        close(listener->sockfd);
        char message_buffer[512];
        memset(message_buffer, 0, 512);
        int read_result = read(client_sockfd, message_buffer, 511);
        printf("Recieved Message from Client\n");

        if (read_result < 0) {
          printf("ERROR reading value");
          return 1;
        } else if (read_result == 0) {
          exit(EXIT_SUCCESS);
        }

        int child_message_sockfd = CreateDomainSock();

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "ipc_socket", sizeof(addr.sun_path) - 1);

        if (connect(child_message_sockfd, (struct sockaddr *)&addr,
                    sizeof(addr)) < 0) {
          perror("Connection to Message Socket Failed\n");
          close(child_message_sockfd);
          exit(EXIT_FAILURE);
        }

        UserMessage message = {.user_id = curr_client_index + 1};
        strncpy(message.message, message_buffer, sizeof(message.message) - 1);
        message.message[sizeof(message.message) - 1] = '\0';

        if (send(child_message_sockfd, &message, sizeof(message), 0) == -1) {
          perror("Failed To Send Message\n");
        }
      }
      curr_client_index++;
    }

    if (FD_ISSET(message_sockfd, &read_fds)) {
      UserMessage message;
      ssize_t bytes_recieved;

      // Create a child domain socket to read from parent
      struct sockaddr_un message_acceptor_addr;
      socklen_t addr_len = sizeof(message_acceptor_addr);

      int message_acceptor_sockfd = accept(
          message_sockfd, (struct sockaddr *)&message_acceptor_addr, &addr_len);
      if (message_acceptor_sockfd == -1) {
        perror("Accept Failed\n");
        continue;
      }

      // Use child socket to read message from client
      if ((bytes_recieved = read(message_acceptor_sockfd, &message,
                                 sizeof(message))) == -1) {
        perror("Failed To Read Message");
        printf("%d\n", errno);
        close(message_acceptor_sockfd);
        continue;
      } else if (bytes_recieved != sizeof(message)) {
        perror("Failed to Read full message");
        close(message_acceptor_sockfd);
        continue;
      }

      // send message to all users but the one who sent it
      char full_message[sizeof(message.message) + 10];
      snprintf(full_message, sizeof(full_message), "User %d: %s",
               message.user_id, message.message);
      for (int i = 0; i < curr_client_index; i++) {
        if (message.user_id - 1 == i) {
          continue;
        }
        send(client_fds[i], full_message, strlen(full_message), 0);
      }

      close(message_acceptor_sockfd);
    }
  }

  close(listener->sockfd);

  return 0;
}
