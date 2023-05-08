#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include "common.h"
#include "helpers.h"

class Sockets
{

public:
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;

  Sockets(char *address, uint16_t port)
  {
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    int rc = inet_pton(AF_INET, address, &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "IP Server is invalid (inet_pton)");

    memset(&cli_addr, 0, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(port);
    cli_addr.sin_addr.s_addr = INADDR_ANY;
  };

  Sockets(uint16_t port)
  {
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
  }

  int init_socket(int type_of_socket, int optname)
  {
    int sockfd = -1;

    // Obtinem un socket TCP pentru conectarea la server
    sockfd = socket(PF_INET, type_of_socket, 0);
    DIE(sockfd < 0, "socket");

    int enable = 1;
    int rc = setsockopt(sockfd, SOL_SOCKET, optname, &enable, sizeof(int));
    DIE(rc < 0, "setsockopt(TCP_NODELAY) failed");

    return sockfd;
  };

  int init_tcp_listener()
  {
    int sockfd = init_socket(SOCK_STREAM, SO_REUSEADDR);
    // Asociem adresa serverului cu socketul creat folosind bind
    int rc = bind(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind tcp");

    // Ascultam pentru conexiuni pe socketul TCP
    rc = listen(sockfd, MAX_CONNECTIONS - 2);
    DIE(rc < 0, "listen tcp");

    return sockfd;
  }

  int init_udp_listener()
  {
    int sockfd = init_socket(SOCK_DGRAM, SO_REUSEADDR);
    // Asociem adresa serverului cu socketul creat folosind bind
    int rc = bind(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind udp");

    return sockfd;
  }

  int init_tcp_client(int type_of_socket, int optname)
  {
    int sockfd = init_socket(type_of_socket, optname);

    // Ne conectăm la server
    int rc = connect(sockfd, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
    DIE(rc < 0, "connect");

    return sockfd;
  }
};