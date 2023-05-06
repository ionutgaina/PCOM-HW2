/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP si mulplixare
 * client.c
 */

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

void run_client(int sockfd, char *client_id)
{
  char buf[MSG_MAXSIZE + 1];
  memset(buf, 0, MSG_MAXSIZE + 1);

  struct chat_packet sent_packet;
  struct chat_packet recv_packet;

  // Send the client ID to the server
  sent_packet.len = strlen(client_id) + 1;
  strcpy(sent_packet.message, client_id);

  // Use send_all function to send the pachet to the server.
  send_all(sockfd, &sent_packet, sizeof(sent_packet));

  // Receive a message and show it's content
  int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
  DIE(rc <= 0, "recv_all");

  printf("%s\n", recv_packet.message);
  while (fgets(buf, sizeof(buf), stdin) && !isspace(buf[0]))
  {
    sent_packet.len = strlen(buf) + 1;
    strcpy(sent_packet.message, buf);

    // Use send_all function to send the pachet to the server.
    send_all(sockfd, &sent_packet, sizeof(sent_packet));

    // Receive a message and show it's content
    int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
    if (rc <= 0)
    {
      break;
    }

    printf("%s\n", recv_packet.message);
  }
}

int main(int argc, char *argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  int sockfd = -1;

  if (argc != 4)
  {
    printf("\n Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", argv[0]);
    return 1;
  }

  // Parsam ID-ul clientului
  char *client_id = argv[1];
  DIE(strlen(client_id) > 10 && strlen(client_id) == 0, "Client ID is invalid");

  std::cout << "Client ID: " << client_id << "\n";

  // Parsam port-ul ca un numar
  uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  std::cout << "PORT: " << port << "\n";

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare

  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(serv_addr);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "IP Server is invalid (inet_pton)");

  std::cout << "IP: " << argv[2] << "\n";

  // Obtinem un socket TCP pentru conectarea la server
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  // Ne conectăm la server
  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");

  run_client(sockfd, client_id);

  // Inchidem conexiunea si socketul creat
  close(sockfd);

  return 0;
}
