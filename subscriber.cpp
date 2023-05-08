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
#include <netinet/tcp.h>

#include "common.h"
#include "helpers.h"
#include "./sockets.cpp"

void run_client(struct pollfd poll_fds[], int num_sockets, char *client_id);

int main(int argc, char *argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  int tcp_sock = -1;

  DIE(argc != 4, "Usage: ./subscriber <ID_Client> <IP_Server> <Port_Server>");

  // Parsam ID-ul clientului
  char *client_id = argv[1];
  DIE(strlen(client_id) > 10 || strlen(client_id) == 0, "Client ID is invalid");

  // Parsam port-ul ca un numar
  uint16_t port;
  int ret = sscanf(argv[3], "%hu", &port);
  DIE(ret != 1, "Given port is invalid");

  Sockets socket = Sockets(argv[2], port);

  // Deschidem un socket TCP
  tcp_sock = socket.init_tcp_client(SOCK_STREAM, TCP_NODELAY);

  struct pollfd poll_fds[MAX_CONNECTIONS];

  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;

  poll_fds[1].fd = tcp_sock;
  poll_fds[1].events = POLLIN;

  int num_sockets = 2;

  run_client(poll_fds, num_sockets, client_id);

  // Inchidem conexiunea si socketul creat
  close(tcp_sock);

  return 0;
}

void run_client(struct pollfd poll_fds[], int num_sockets, char *client_id)
{
  int ret;
  char buf[BUFSIZ];
  memset(buf, 0, BUFSIZ);

  struct packet send_packet;

  send_packet.header.len = strlen(client_id);
  send_packet.header.message_type = 0;

  strcpy(send_packet.content, client_id);

  // trimite id-ul spre server
  ret = send(poll_fds[1].fd, &send_packet, sizeof(send_packet), 0);
  DIE(ret < 0, "send");

  while (1)
  {
    ret = poll(poll_fds, num_sockets, -1);
    DIE(ret < 0, "poll");

    for (int i = 0; i < num_sockets; i++)
    {
      if (i == 0 && (poll_fds[0].revents & POLLIN))
      {
        std::cin.getline(buf, sizeof(buf));

        // sterge newline-ul
        if (buf[strlen(buf) - 1] == '\n')
        {
          buf[strlen(buf) - 1] = '\0';
        }

        if (strlen(buf) == 0)
        {
          std::cerr << "Clientul poate trimite doar mesaje de tipul 'exit', 'subscribe <TOPIC> <SF>', 'unsubscribe <TOPIC>'\n";
          continue;
        }

        char *command = strtok(buf, " ");
        /*
          EXIT
        */
        if (strncmp(command, "exit", 4) == 0)
        {
          return;
        }
        /*
          UNSUBSCRIBE
        */
        if (strncmp(command, "unsubscribe", 11) == 0)
        {
          struct subscribe_packet unsubscription_packet;
          char *topic = strtok(NULL, " ");
          if (topic == NULL)
          {
            std::cerr << "Topic nu este definit\n";
            break;
          }

          if (strlen(topic) > 50)
          {
            std::cerr << "Topicul este prea lung\n";
            break;
          }

          if (strtok(NULL, " ") != NULL)
          {
            std::cerr << "Prea multe argumente -> unsubscribe <TOPIC>\n";
            break;
          }

          strcpy(unsubscription_packet.command, "unsubscribe");
          strcpy(unsubscription_packet.topic, topic);
          unsubscription_packet.sf = -1;

          send_packet.header.len = sizeof(unsubscription_packet);
          memcpy(send_packet.content, &unsubscription_packet, sizeof(unsubscription_packet));
          send_packet.header.message_type = 1;

          ret = send_all(poll_fds[1].fd, &send_packet, sizeof(send_packet));
          DIE(ret < 0, "send");

          std::cout << "Unsubscribed from topic.\n";
          continue;
        }
        /*
          SUBSCRIBE
        */
        if (strncmp(command, "subscribe", 9) == 0)
        {
          struct subscribe_packet subscription_packet;
          char *topic = strtok(NULL, " ");
          if (topic == NULL)
          {
            std::cerr << "Topic nu este definit\n";
            break;
          }

          if (strlen(topic) > 50)
          {
            std::cerr << "Topicul este prea lung\n";
            break;
          }
          char *sf_string = strtok(NULL, " ");

          if (sf_string == NULL)
          {
            std::cerr << "SF nu este definit\n";
            break;
          }

          int sf = atoi(sf_string);
          if (sf != 0 && sf != 1)
          {
            std::cerr << "SF nu este 0 sau 1\n";
            break;
          }

          if (strtok(NULL, " ") != NULL)
          {
            std::cerr << "Prea multe argumente -> subscribe <TOPIC> <SF>\n";
            break;
          }

          strcpy(subscription_packet.topic, topic);
          strcpy(subscription_packet.command, "subscribe");
          strcpy(subscription_packet.id, client_id);
          subscription_packet.sf = sf;

          send_packet.header.len = sizeof(subscription_packet);
          memcpy(send_packet.content, &subscription_packet, sizeof(subscription_packet));

          send_packet.header.message_type = 1;

          ret = send_all(poll_fds[1].fd, &send_packet, sizeof(send_packet));
          DIE(ret < 0, "send");

          std::cout << "Subscribed to topic.\n";
          continue;
        }

        std::cerr << "Clientul poate trimite doar mesaje de tipul 'exit', 'subscribe <TOPIC> <SF>', 'unsubscribe <TOPIC>'\n";
        continue;
      }

      if (i == 1 && (poll_fds[1].revents & POLLIN))
      {
        struct packet recv_packet;
        ret = recv_all(poll_fds[1].fd, &recv_packet, sizeof(recv_packet));
        if (ret <= 0)
        {
          return;
        }

        if (recv_packet.header.message_type == 0)
        {
          std::cout << recv_packet.content << "\n";
          continue;
        }
        std::cerr << "Serverul a trimis un mesaj neinteles: " << recv_packet.content << "\n";
        continue;
      }
    }
  }
}
