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

void run_client(struct pollfd poll_fds[], int num_clients, char *client_id)
{

  int rc, ret;
  char buf[BUFSIZ];
  memset(buf, 0, BUFSIZ);

  struct packet sent_packet;
  struct packet recv_packet;

  // Send the client ID to the server
  sent_packet.len = strlen(client_id);
  strcpy(sent_packet.message, client_id);

  // Use send function to send the pachet to the server.
  rc = send(poll_fds[1].fd, &sent_packet, sizeof(sent_packet), 0);

  // Receive a message and show it's content
  rc = recv(poll_fds[1].fd, &recv_packet, sizeof(recv_packet), 0);
  DIE(rc <= 0, "recv");

  if (strncmp(recv_packet.message, "already connected", 17) == 0)
  {
    DIE(1, "Clientul este deja conectat");
  }
  else if (strncmp(recv_packet.message, client_id, strlen(client_id)) != 0)
  {
    DIE(1, "Serverul nu a primit ID-ul clientului");
  }

  /*
        DEJA CONECTAT
  */

  while (1)
  {
    ret = poll(poll_fds, num_clients, -1);
    DIE(ret < 0, "poll");

    for (int i = 0; i < num_clients; i++)
    {

      if (i == 0 && poll_fds[0].revents & POLLIN)
      {
        memset(buf, 0, BUFSIZ);
        std::cin.getline(buf, sizeof(buf));

        // sterge newline-ul
        if (buf[strlen(buf) - 1] == '\n')
        {
          buf[strlen(buf) - 1] = '\0';
        }

        sent_packet.len = strlen(buf);
        strcpy(sent_packet.message, buf);

        /*
                 UNSUBSCRIBE
        */

        // copy buf
        char buf_copy[BUFSIZ];
        strcpy(buf_copy, buf);

        if (strlen(buf_copy) == 0)
        {
          std::cerr << "Clientul poate trimite doar mesaje de tipul 'exit', 'subscribe <TOPIC> <SF>', 'unsubscribe <TOPIC>'\n";
          break;
        }

        char *command = strtok(buf_copy, " ");
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

          strcpy(unsubscription_packet.topic, topic);

          // remove from the topic the newline
          if (unsubscription_packet.topic[strlen(unsubscription_packet.topic) - 1] == '\n')
          {
            unsubscription_packet.topic[strlen(unsubscription_packet.topic) - 1] = '\0';
          }

          strcpy(unsubscription_packet.command, "unsubscribe");
          strcpy(unsubscription_packet.id, client_id);
          unsubscription_packet.sf = -1;

          sent_packet.len = sizeof(unsubscription_packet);
          memcpy(sent_packet.message, &unsubscription_packet, sizeof(unsubscription_packet));
          sent_packet.message_type = 1;
        }
        else
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
            subscription_packet.sf = sf;
            strcpy(subscription_packet.id, client_id);

            sent_packet.len = sizeof(subscription_packet);
            memcpy(sent_packet.message, &subscription_packet, sizeof(subscription_packet));

            sent_packet.message_type = 1;
          }
          else if (strncmp(command, "exit", 4) == 0)
          {
            return;
          }
          else
          {
            std::cerr << "Clientul poate trimite doar mesaje de tipul 'exit', 'subscribe <TOPIC> <SF>', 'unsubscribe <TOPIC>'\n";
            break;
          }

        // Use send function to send the pachet to the server.
        rc = send(poll_fds[1].fd, &sent_packet, sizeof(sent_packet), 0);
        DIE(rc <= 0, "send");
        break;
      }

      if (i == 1 && poll_fds[1].revents & POLLIN)
      {
        // Receive a message and show it's content
        rc = recv(poll_fds[1].fd, &recv_packet, sizeof(recv_packet), 0);
        if (rc <= 0)
        {
          return;
        }

        if (recv_packet.message_type == 2)
        {
          std::cout << recv_packet.message << "\n";
          break;
        }
        /*
              UNSUBSCRIBE RESPONSE
        */
        if (strncmp(recv_packet.message, "unsubscribe", 11) == 0)
        {
          std::cout << "Unsubscribed from topic.\n";
          break;
        }

        /*
              SUBSCRIBE RESPONSE
        */
        if (strncmp(recv_packet.message, "subscribe", 9) == 0)
        {
          std::cout << "Subscribed to topic.\n";
          break;
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  int sockfd = -1;

  DIE(argc != 4, "Usage: ./subscriber <ID_Client> <IP_Server> <Port_Server>");

  // Parsam ID-ul clientului
  char *client_id = argv[1];
  DIE(strlen(client_id) > 10 && strlen(client_id) == 0, "Client ID is invalid");

  // Parsam port-ul ca un numar
  uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare

  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(serv_addr);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "IP Server is invalid (inet_pton)");

  // Obtinem un socket TCP pentru conectarea la server
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  // Ne conectăm la server
  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");

  struct pollfd poll_fds[MAX_CONNECTIONS];

  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;

  poll_fds[1].fd = sockfd;
  poll_fds[1].events = POLLIN;

  int num_clients = 2;

  run_client(poll_fds, num_clients, client_id);

  // Inchidem conexiunea si socketul creat
  close(sockfd);

  return 0;
}
