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

  int rc;
  char buf[BUFSIZ];
  memset(buf, 0, BUFSIZ);

  struct packet sent_packet;
  struct packet recv_packet;

  // Send the client ID to the server
  sent_packet.len = strlen(client_id);
  strcpy(sent_packet.message, client_id);

  // Use send function to send the pachet to the server.
  rc = send(sockfd, &sent_packet, sizeof(sent_packet), 0);

  // Receive a message and show it's content
  rc = recv(sockfd, &recv_packet, sizeof(recv_packet), 0);
  DIE(rc <= 0, "recv");
  /*
        DEJA CONECTAT
  */
  if (strncmp(recv_packet.message, "already connected", 17) == 0)
  {
    return;
  }

  while (fgets(buf, sizeof(buf), stdin) && !isspace(buf[0]))
  {
    sent_packet.len = strlen(buf);
    strcpy(sent_packet.message, buf);

    /*
             UNSUBSCRIBE
       */

    // copy buf
    char buf_copy[BUFSIZ];
    strcpy(buf_copy, buf);

    char *command = strtok(buf_copy, " ");
    if (strncmp(command, "unsubscribe", 11) == 0)
    {
      struct subscribe_packet unsubscription_packet;
      char *topic = strtok(NULL, " ");
      if (topic == NULL)
      {
        std::cerr << "Topic nu este definit\n";
        continue;
      }

      if (strlen(topic) > 50)
      {
        std::cerr << "Topicul este prea lung\n";
        continue;
      }

      if (strtok(NULL, " ") != NULL)
      {
        std::cerr << "Prea multe argumente -> unsubscribe <TOPIC>\n";
        continue;
      }

      strcpy(unsubscription_packet.topic, topic);

      // remove from topic the newline
      unsubscription_packet.topic[strlen(unsubscription_packet.topic) - 1] = '\0';

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
          continue;
        }

        if (strlen(topic) > 50)
        {
          std::cerr << "Topicul este prea lung\n";
          continue;
        }
        char *sf_string = strtok(NULL, " ");

        if (sf_string == NULL)
        {
          std::cerr << "SF nu este definit\n";
          continue;
        }

        int sf = atoi(sf_string);
        if (sf != 0 && sf != 1)
        {
          std::cerr << "SF nu este 0 sau 1\n";
          continue;
        }

        if (strtok(NULL, " ") != NULL)
        {
          std::cerr << "Prea multe argumente -> subscribe <TOPIC> <SF>\n";
          continue;
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
        break;
      }
      else
      {
        std::cerr << "Clientul poate trimite doar mesaje de tipul 'exit', 'subscribe <TOPIC> <SF>', 'unsubscribe <TOPIC>'\n";
        continue;
      }

    // Use send function to send the pachet to the server.
    rc = send(sockfd, &sent_packet, sizeof(sent_packet), 0);

    // Receive a message and show it's content
    rc = recv(sockfd, &recv_packet, sizeof(recv_packet), 0);
    if (rc <= 0)
    {
      break;
    }

    if (recv_packet.message_type == 2)
    {
      std::cout << recv_packet.message << "\n";
      continue;
    }
    /*
          UNSUBSCRIBE RESPONSE
    */
    if (strncmp(recv_packet.message, "unsubscribe", 11) == 0)
    {
      std::cout << "Unsubscribed from topic.\n";
      continue;
    }

    /*
          SUBSCRIBE RESPONSE
    */
    if (strncmp(recv_packet.message, "subscribe", 9) == 0)
    {
      std::cout << "Subscribed to topic.\n";
      continue;
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

  run_client(sockfd, client_id);

  // Inchidem conexiunea si socketul creat
  close(sockfd);

  return 0;
}
