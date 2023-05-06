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

#define MAX_CONNECTIONS 32

// Primeste date de pe connfd1 si trimite mesajul receptionat pe connfd2
int receive_and_send(int connfd1, int connfd2, size_t len)
{
  int bytes_received;
  char buffer[len];
  int rc;

  // Primim exact len octeti de la connfd1
  bytes_received = recv_all(connfd1, buffer, len);
  // S-a inchis conexiunea
  if (bytes_received == 0)
  {
    return 0;
  }
  DIE(bytes_received < 0, "recv");

  // Trimitem mesajul catre connfd2
  rc = send_all(connfd2, buffer, len);
  if (rc <= 0)
  {
    perror("send_all");
    return -1;
  }

  return bytes_received;
}

int main(int argc, char *argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  DIE(argc != 2, "Usage: ./server <port>");

  int rc, ret, i;
  std::unordered_map<int, std::string> socket_to_id;

  // Parsam port-ul ca un numar
  uint16_t port;
  rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  /*
          TCP
  */

  // Obtinem un socket TCP pentru receptionarea conexiunilor
  int listen_tcp = socket(AF_INET, SOCK_STREAM, 0);
  DIE(listen_tcp < 0, "tcp socket");

  // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
  // rulam de 2 ori rapid
  int enable = 1;

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
        char buf[MSG_MAXSIZE + 1];
        std::cin.getline(buf, sizeof(buf));
        if (strcmp(buf, "exit") == 0)
        {
          // Inchidem socketii
          for (int num_socket = 1; num_socket < num_clients; num_socket++)
          {
            close(poll_fds[num_socket].fd);
          }
          return 0;
        }
        else
        {
          DIE(1, "Comanda invalida");
        }
      }

      // Eveniment pe socketul UDP
      if (poll_fds[1].revents & POLLIN)
      {
        std::cout << "Cerere de citire pe socketul UDP\n";
      }

      if (i > 1 && (poll_fds[i].revents & POLLIN))
      {
        // std::cout << "Cerere de citire pe socketul TCP " << i << "\n";

        // Eveniment pe socketii TCP
        char buf[MSG_MAXSIZE + 1];
        memset(buf, 0, MSG_MAXSIZE + 1);

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

          rc = recv_all(newsockfd, &recv_packet, sizeof(recv_packet));
          DIE(rc < 0, "recv_all");

          auto it = std::find_if(socket_to_id.begin(), socket_to_id.end(), [&](const std::pair<int, std::string> &p)
                                 { return p.second == recv_packet.message; });
          if (it != socket_to_id.end())
          {
            // daca id-ul este deja folosit
            std::cout << "Client " << recv_packet.message << " already connected.\n";

            strcpy(recv_packet.message, "already connected");
            recv_packet.len = strlen(recv_packet.message);
            ret = send_all(current_socket, &recv_packet, sizeof(recv_packet));
            DIE(ret < 0, "send_all");
            close(newsockfd);
            continue;
          }

          // se adauga noul socket intors de accept() la multimea descriptorilor
          // de citire
          poll_fds[num_clients].fd = newsockfd;
          poll_fds[num_clients].events = POLLIN;
          num_clients++;

          std::cout << "New client " << recv_packet.message << " connected from " << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << ".\n";

          // se adauga in map-ul de id-uri si socketi
          socket_to_id.insert({newsockfd, recv_packet.message});
        }
        else
        {

          rc = recv_all(poll_fds[i].fd, &recv_packet, sizeof(recv_packet));
          DIE(rc < 0, "recv_all");
          current_socket = poll_fds[i].fd;
          /*
                  INCHID CONEXIUNE (EXIT)
          */
          if (strncmp(recv_packet.message, "exit", 4) == 0)
          {
            // conexiunea s-a inchis
            int ID_CLIENT = socket_to_id.find(current_socket)->first;
            std::cout << "Client " << ID_CLIENT << " disconnected.\n";

            // se scoate socketul inchis de la polling
            close(poll_fds[i].fd);
            poll_fds[i].fd = -1;
            num_clients--;
          }
          else
          {
            // afisam mesajul primit
            printf("[client %d] %s\n", poll_fds[i].fd, recv_packet.message);
          }
        }

        // raspundem clientului cu acelasi mesaj
        // (nu conteaza daca se aboneaza un client sau ne scrie ceva)
        ret = send_all(current_socket, &recv_packet, sizeof(recv_packet));
        DIE(ret < 0, "send_all");
      }
    }
  }
  return 0;
}
