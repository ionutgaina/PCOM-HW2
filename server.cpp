/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <netinet/tcp.h>

#include <unordered_map>
#include <algorithm>

#include "common.h"
#include "helpers.h"
#include "./user.cpp"
#include "./data_parser.cpp"

#define MAX_CONNECTIONS 32

int main(int argc, char *argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  DIE(argc != 2, "Usage: ./server <port>");

  int rc, ret, i;
  std::unordered_map<std::string, Client_TCP *> id_to_client;

  // Parsam port-ul ca un numar
  uint16_t port;
  rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1, "Port-ul dat nu este valid");

  /*
          TCP
  */

  // Obtinem un socket TCP pentru receptionarea conexiunilor
  int listen_tcp = socket(AF_INET, SOCK_STREAM, 0);
  DIE(listen_tcp < 0, "tcp socket");

  // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
  // rulam de 2 ori rapid
  int enable = 1;

  // TODO: setam optiunea de socket TCP_NODELAY
  rc = setsockopt(listen_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  DIE(rc < 0, "setsockopt(TCP_NODELAY) failed");

  /*
          ADDRESE PENTRU SOCKETI
  */
  // Completăm in tcp_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  struct sockaddr_in tcp_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  // Setam familia de adrese pentru socket-ul TCP
  memset(&tcp_addr, 0, socket_len);
  tcp_addr.sin_family = AF_INET;
  tcp_addr.sin_addr.s_addr = INADDR_ANY;
  tcp_addr.sin_port = htons(port);

  // Asociem adresa serverului cu socketul creat folosind bind
  rc = bind(listen_tcp, (const struct sockaddr *)&tcp_addr, sizeof(tcp_addr));
  DIE(rc < 0, "bind");

  // Ascultam pentru conexiuni pe socketul TCP
  // -2 pentru ca o conexiune va fi UDP și una pentru STDIN
  rc = listen(listen_tcp, MAX_CONNECTIONS - 2);
  DIE(rc < 0, "listen");

  /*
        UDP
  */

  // Obtinem un socket UDP pentru receptionarea mesajelor
  int listen_udp = socket(PF_INET, SOCK_DGRAM, 0);
  DIE(listen_udp < 0, "udp socket");

  rc = setsockopt(listen_udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  DIE(rc < 0, "setsockopt(SO_REUSEADDR) failed");

  // Setam familia de adrese pentru socket-ul UDP
  struct sockaddr_in udp_addr;
  memset(&udp_addr, 0, socket_len);
  udp_addr.sin_family = AF_INET;
  udp_addr.sin_addr.s_addr = INADDR_ANY;
  udp_addr.sin_port = htons(port);

  rc = bind(listen_udp, (const struct sockaddr *)&udp_addr, sizeof(udp_addr));
  DIE(rc < 0, "bind");

  /*
          POLL
  */

  // Initializam multimea de descriptori de citire
  struct pollfd poll_fds[MAX_CONNECTIONS];

  // Adaugam socketii TCP, UDP si STDIN la multimea de descriptori
  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;

  poll_fds[1].fd = listen_udp;
  poll_fds[1].events = POLLIN;

  poll_fds[2].fd = listen_tcp;
  poll_fds[2].events = POLLIN;

  int num_clients = 3;

  /*
          ASCULTARE
  */

  while (1)
  {
    ret = poll(poll_fds, num_clients, -1);
    DIE(ret < 0, "eroare la poll");

    for (i = 0; i < num_clients; i++)
    {
      // Eveniment pe stdin
      if (poll_fds[0].revents & POLLIN)
      {
        char buf[BUFSIZ];
        std::cin.getline(buf, sizeof(buf));
        if (strcmp(buf, "exit") == 0)
        {
          // Inchidem socketii
          for (int num_socket = 1; num_socket < num_clients; num_socket++)
          {
            close(poll_fds[num_socket].fd);
            // to do free all the memory

            auto it = std::find_if(id_to_client.begin(), id_to_client.end(), [&](const std::pair<std::string, Client_TCP *> &p)
                                   { return p.second->socket == poll_fds[num_socket].fd; });

            if (it != id_to_client.end())
            {
              delete it->second;
              id_to_client.erase(it);
            }
          }
          return 0;
        }
        else
        {
          std::cerr << "Comanda invalidă, se permite doar comanda 'exit'\n";
          continue;
        }
      }

      // Eveniment pe socketul UDP
      if (poll_fds[1].revents & POLLIN)
      {
        struct udp_packet udp_packet;
        memset(&udp_packet, 0, sizeof(udp_packet));

        struct sockaddr_in udp_client;
        socklen_t len = sizeof(udp_client);

        // Citim mesajul de pe socketul UDP
        rc = recvfrom(listen_udp, &udp_packet, sizeof(udp_packet), 0, (struct sockaddr *)&udp_client, &len);
        DIE(rc < 0, "recvfrom");

        // printf("UDP topic %s\n", udp_packet.topic);
        // printf("Data type %d\n", udp_packet.data_type);
        // printf("content %s\n", udp_packet.content);
        // printf("UDP port %d\n", ntohs(udp_client.sin_port));
        // printf("UDP IP %s\n", inet_ntoa(udp_client.sin_addr));

        Data_Parser data_parser;
        std::string result = data_parser.parse_udp_message(udp_packet, udp_client);

        std::cout << result << "\n";
      }

      if (i > 1 && (poll_fds[i].revents & POLLIN))
      {

        // std::cout << "Cerere de citire pe socketul TCP << " << poll_fds[i].fd << "\n";
        // Eveniment pe socketii TCP
        char buf[BUFSIZ];
        memset(buf, 0, BUFSIZ);

        struct packet recv_packet;

        int current_socket = poll_fds[i].fd;

        if (poll_fds[i].fd == listen_tcp)
        {

          // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
          // pe care serverul o accepta
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);
          int newsockfd =
              accept(listen_tcp, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "accept");
          current_socket = newsockfd;

          rc = recv(newsockfd, &recv_packet, sizeof(recv_packet), 0);
          DIE(rc < 0, "recv");

          // se verifica daca id-ul este deja folosit
          auto it = id_to_client.find(recv_packet.message);

          if (it != id_to_client.end())
          {
            if (it->second->socket != -1)
            {
              // daca id-ul este deja folosit
              std::cout << "Client " << recv_packet.message << " already connected.\n";

              strcpy(recv_packet.message, "already connected");
              recv_packet.len = strlen(recv_packet.message);
              ret = send(current_socket, &recv_packet, sizeof(recv_packet), 0);
              DIE(ret < 0, "send");
              close(newsockfd);
              continue;
            }

            // daca id-ul a fost folosit si s-a deconectat
            std::cout << "Client reconectat " << recv_packet.message << ".\n";
            it->second->socket = newsockfd;
            // TODO trimite mesajele pe care trebuia sa le primeasca la topic-urile cu SF1
          }
          else
          {
            // se adauga in map-ul de id-uri si socketi
            Client_TCP *new_client = new Client_TCP(newsockfd);
            id_to_client.insert({recv_packet.message, new_client});
          }

          // se adauga noul socket intors de accept() la multimea descriptorilor
          // de citire
          poll_fds[num_clients].fd = newsockfd;
          poll_fds[num_clients].events = POLLIN;
          ++num_clients;

          std::cout << "New client " << recv_packet.message << " connected from " << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << ".\n";
        }
        else
        {

          rc = recv(poll_fds[i].fd, &recv_packet, sizeof(recv_packet), 0);
          DIE(rc < 0, "recv");
          current_socket = poll_fds[i].fd;
          /*
                  INCHID CONEXIUNE (EXIT)
          */
          if (rc == 0)
          {
            // conexiunea s-a inchis
            auto it = std::find_if(id_to_client.begin(), id_to_client.end(), [&](const std::pair<std::string, Client_TCP *> &p)
                                   { return p.second->socket == current_socket; });

            DIE(it == id_to_client.end(), "Eroare la gasirea socketului in map");

            std::string ID_CLIENT = it->first;
            std::cout << "Client " << ID_CLIENT << " disconnected.\n";

            // se scoate socketul inchis de la polling
            it->second->socket = -1;
            close(poll_fds[i].fd);

            // ordonez vectorul de polling
            for (int j = i; j < num_clients - 1; j++)
            {
              poll_fds[j] = poll_fds[j + 1];
            }

            --num_clients;
            continue;
          }

          /*
                  SUBSCRIBE
          */
          else if (recv_packet.message_type == 1 && recv_packet.len == sizeof(subscribe_packet))
          {

            struct subscribe_packet *subscribe_packet = (struct subscribe_packet *)&recv_packet.message;

            auto it = id_to_client.find(subscribe_packet->id);

            if (it == id_to_client.end())
            {
              std::cerr << "SUBSCRIBE: Clientul cu ID-ul " << subscribe_packet->id << " nu este conectat\n";
              continue;
            }

            Client_TCP *client = it->second;

            if (strncmp(subscribe_packet->command, "unsubscribe", 11) == 0)
            {
              // se sterge topicul din map-ul de topicuri
              client->unsubscribe_topic(subscribe_packet->topic);
            }
            else if (strncmp(subscribe_packet->command, "subscribe", 9) == 0)
            {
              // se adauga topicul in map-ul de topicuri sau se actualizeaza sf-ul
              client->subscribe_topic(subscribe_packet->topic, subscribe_packet->sf);
            }
            else
            {
              std::cerr << "SUBSCRIBE PACKET: Comanda gresita\n";
              continue;
            }
          }
        }

        // raspundem clientului cu acelasi mesaj
        // (nu conteaza daca se aboneaza un client sau ne scrie ceva)
        ret = send(current_socket, &recv_packet, sizeof(recv_packet), 0);
        DIE(ret < 0, "send");
      }
    }
  }
  return 0;
}
