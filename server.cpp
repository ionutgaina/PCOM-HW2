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
#include "./sockets.cpp"

void run_server(struct pollfd poll_fds[], int num_sockets);
std::string udp_response(int socketid);

int main(int argc, char *argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  DIE(argc != 2, "Usage: ./server <port>");

  int ret;

  // Parsam port-ul ca un numar
  uint16_t port;
  ret = sscanf(argv[1], "%hu", &port);
  DIE(ret != 1, "Port-ul dat nu este valid");

  Sockets socket = Sockets(port);
  /*
          TCP
  */

  // Obtinem un socket TCP pentru receptionarea conexiunilor
  int listen_tcp = socket.init_tcp_listener();

  /*
        UDP
  */

  // Obtinem un socket UDP pentru receptionarea mesajelor
  int listen_udp = socket.init_udp_listener();
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

  int num_sockets = 3;

  run_server(poll_fds, num_sockets);

  // Inchidem socketii
  for (int i = 1; i < num_sockets; i++)
  {
    close(poll_fds[i].fd);
  }

  return 0;
}

void run_server(struct pollfd poll_fds[], int num_sockets)
{
  int ret;
  std::unordered_map<std::string, Client_TCP *> id_to_client;
  std::unordered_map<std::string, std::vector<std::string>> topic_to_id;

  while (1)
  {
    ret = poll(poll_fds, num_sockets, -1);
    DIE(ret < 0, "eroare la poll");

    for (int i = 0; i < num_sockets; i++)
    {
      /*
        STDIN
      */
      if ((poll_fds[i].revents & POLLIN) && (poll_fds[i].fd == STDIN_FILENO))
      {
        // std::cout << "Eveniment pe STDIN\n";
        char buf[BUFSIZ];
        scanf("%s", buf);

        if (strcmp(buf, "exit") == 0)
        {

          // eliberam memoria
          for (int i = 3; i < num_sockets; i++)
          {
            auto it = std::find_if(id_to_client.begin(), id_to_client.end(), [&](const std::pair<std::string, Client_TCP *> &p)
                                   { return p.second->socket == poll_fds[i].fd; });

            if (it != id_to_client.end())
            {
              delete it->second;
              id_to_client.erase(it);
            }
          }
          return;
        }
        else
        {
          std::cerr << "Comanda invalidă, se permite doar comanda 'exit'\n";
          continue;
        }
      }
      else
        /*
          UDP
        */
        // Eveniment pe socketul UDP
        if ((poll_fds[i].revents & POLLIN) && (i == 1))
        {
          // std::cout << "Eveniment pe socketul UDP\n";
          struct udp_packet udp_packet;
          memset(&udp_packet, 0, sizeof(udp_packet));

          struct sockaddr_in udp_client;
          socklen_t len = sizeof(udp_client);

          // Citim mesajul de pe socketul UDP
          int ret = recvfrom(poll_fds[1].fd, &udp_packet, sizeof(udp_packet), 0, (struct sockaddr *)&udp_client, &len);
          DIE(ret < 0, "recvfrom");

          Data_Parser data_parser;
          std::string result = data_parser.parse_udp_message(udp_packet, udp_client);

          if (result == "")
          {
            std::cerr << "Mesajul de la clientul UDP nu a putut fi parsat\n";
            continue;
          }

          // Cautam in map-ul de topicuri clientul care are topicul respectiv
          for (auto it = id_to_client.begin(); it != id_to_client.end(); it++)
          {
            if (it->second->has_topic(udp_packet.topic))
            {
              int socket = it->second->socket;
              if (socket == -1)
              {
                // clientul are topicul si are sf = 1, deci trebuie sa salveze mesajul
                if (it->second->get_sf(udp_packet.topic) == 1)
                {
                  it->second->add_message(udp_packet.topic, result.c_str());
                }
              }
              else
              {
                // clientul are topicul, trimitem mesajul
                struct packet send_packet;
                memset(&send_packet, 0, sizeof(send_packet));
                send_packet.header.len = result.size();
                send_packet.header.message_type = 0;

                strcpy(send_packet.content, result.c_str());

                ret = send_all(socket, &send_packet, sizeof(send_packet));
                DIE(ret < 0, "send");
              }
            }
          }
        }
        else

          /*
            TCP
          */

          if ((i == 2) && (poll_fds[i].revents & POLLIN))
          {
            // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
            // pe care serverul o accepta
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);

            struct packet recv_packet;

            int newsockfd =
                accept(poll_fds[2].fd, (struct sockaddr *)&cli_addr, &cli_len);
            DIE(newsockfd < 0, "accept");

            int enable = 1;
            int rc = setsockopt(newsockfd, SOL_SOCKET, TCP_NODELAY, &enable, sizeof(int));
            DIE(rc < 0, "setsockopt(TCP_NODELAY) failed");

            ret = recv_all(newsockfd, &recv_packet, sizeof(recv_packet));
            DIE(ret < 0, "recv_all");

            // se verifica daca id-ul este deja folosit

            std::string id = std::string(recv_packet.content, recv_packet.header.len);

            auto it = id_to_client.find(id);

            if (it != id_to_client.end())
            {
              if (it->second->socket != -1)
              {
                // daca id-ul este deja folosit
                std::cout << "Client " << id << " already connected.\n";
                close(newsockfd);
                continue;
              }

              // reconectat
              it->second->socket = newsockfd;
              // trimite mesajele pe care trebuia sa le primeasca la topic-urile cu SF1
              std::vector<std::string> messages = it->second->get_messages_from_topics();

              for (auto &message : messages)
              {
                struct packet send_packet;
                memset(&send_packet, 0, sizeof(send_packet));
                send_packet.header.len = message.size();
                send_packet.header.message_type = 0;

                strcpy(send_packet.content, message.c_str());

                ret = send_all(newsockfd, &send_packet, sizeof(send_packet));
                DIE(ret < 0, "send");
              }
            }
            else
            {
              // se adauga in map-ul de id-uri si socketi
              Client_TCP *new_client = new Client_TCP(newsockfd);
              DIE(new_client == nullptr, "new_client");
              id_to_client.insert({id, new_client});
            }

            // se adauga noul socket intors de accept() la multimea descriptorilor
            // de citire
            poll_fds[num_sockets].fd = newsockfd;
            poll_fds[num_sockets].events = POLLIN;
            poll_fds[num_sockets].revents = 0;
            ++num_sockets;

            std::cout << "New client " << id << " connected from " << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << ".\n";
          }
          else if ((poll_fds[i].revents & POLLIN) && (i > 2))
          {
            // std::cout << "Eveniment pe socketul TCP\n";
            struct packet recv_packet;
            ret = recv_all(poll_fds[i].fd, &recv_packet, sizeof(recv_packet));
            DIE(ret < 0, "recv_all");
            /*
                    INCHID CONEXIUNE (EXIT)
            */
            if (ret == 0)
            {
              // conexiunea s-a inchis
              auto it = std::find_if(id_to_client.begin(), id_to_client.end(), [&](const std::pair<std::string, Client_TCP *> &p)
                                     { return p.second->socket == poll_fds[i].fd; });

              DIE(it == id_to_client.end(), "Eroare la gasirea socketului in map");

              std::string ID_CLIENT = it->first;
              std::cout << "Client " << ID_CLIENT << " disconnected.\n";

              // se scoate socketul inchis de la polling
              it->second->socket = -1;
              close(poll_fds[i].fd);

              // ordonez vectorul de polling
              for (int j = i; j < num_sockets - 1; j++)
              {
                poll_fds[j] = poll_fds[j + 1];
              }

              num_sockets--;
            }
            /*
                    SUBSCRIBE
            */
            else if (recv_packet.header.message_type == 1 && recv_packet.header.len == sizeof(subscribe_packet))
            {

              struct subscribe_packet *subscribe_packet = (struct subscribe_packet *)&recv_packet.content;

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
    }
  }
}
