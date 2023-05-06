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

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32

// Primeste date de pe connfd1 si trimite mesajul receptionat pe connfd2
int receive_and_send(int connfd1, int connfd2, size_t len)
{
  int bytes_received;
  char buffer[len];

  // Primim exact len octeti de la connfd1
  bytes_received = recv_all(connfd1, buffer, len);
  // S-a inchis conexiunea
  if (bytes_received == 0)
  {
    return 0;
  }
  DIE(bytes_received < 0, "recv");

  // Trimitem mesajul catre connfd2
  int rc = send_all(connfd2, buffer, len);
  if (rc <= 0)
  {
    perror("send_all");
    return -1;
  }

  return bytes_received;
}

void run_chat_server(int listenfd)
{
  struct sockaddr_in client_addr1;
  struct sockaddr_in client_addr2;
  socklen_t clen1 = sizeof(client_addr1);
  socklen_t clen2 = sizeof(client_addr2);

  int connfd1 = -1;
  int connfd2 = -1;
  int rc;

  // Setam socket-ul listenfd pentru ascultare
  rc = listen(listenfd, 2);
  DIE(rc < 0, "listen");

  // Acceptam doua conexiuni
  printf("Astept conectarea primului client...\n");
  connfd1 = accept(listenfd, (struct sockaddr *)&client_addr1, &clen1);
  DIE(connfd1 < 0, "accept");

  printf("Astept connectarea clientului 2...\n");

  connfd2 = accept(listenfd, (struct sockaddr *)&client_addr2, &clen2);
  DIE(connfd2 < 0, "accept");

  while (1)
  {
    // Primim de la primul client, trimitem catre al 2lea
    printf("Primesc de la 1 si trimit catre 2...\n");
    int rc = receive_and_send(connfd1, connfd2, sizeof(struct chat_packet));
    if (rc <= 0)
    {
      break;
    }

    rc = receive_and_send(connfd2, connfd1, sizeof(struct chat_packet));
    if (rc <= 0)
    {
      break;
    }
  }

  // Inchidem conexiunile si socketii creati
  close(connfd1);
  close(connfd2);
}

void run_chat_multi_server(int listenfd)
{

  struct pollfd poll_fds[MAX_CONNECTIONS];
  int num_clients = 1;
  int rc;

  struct chat_packet received_packet;

  // Setam socket-ul listenfd pentru ascultare
  rc = listen(listenfd, MAX_CONNECTIONS);
  DIE(rc < 0, "listen");

  // se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
  // multimea read_fds
  poll_fds[0].fd = listenfd;
  poll_fds[0].events = POLLIN;

  while (1)
  {

    rc = poll(poll_fds, num_clients, -1);
    DIE(rc < 0, "poll");

    for (int i = 0; i < num_clients; i++)
    {
      if (poll_fds[i].revents & POLLIN)
      {
        if (poll_fds[i].fd == listenfd)
        {
          // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
          // pe care serverul o accepta
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);
          int newsockfd =
              accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "accept");

          // se adauga noul socket intors de accept() la multimea descriptorilor
          // de citire
          poll_fds[num_clients].fd = newsockfd;
          poll_fds[num_clients].events = POLLIN;
          num_clients++;

          printf("Noua conexiune de la %s, port %d, socket client %d\n",
                 inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port),
                 newsockfd);
        }
        else
        {
          // s-au primit date pe unul din socketii de client,
          // asa ca serverul trebuie sa le receptioneze
          int rc = recv_all(poll_fds[i].fd, &received_packet,
                            sizeof(received_packet));
          DIE(rc < 0, "recv");

          if (rc == 0)
          {
            // conexiunea s-a inchis
            printf("Socket-ul client %d a inchis conexiunea\n", i);
            close(poll_fds[i].fd);

            // se scoate din multimea de citire socketul inchis
            for (int j = i; j < num_clients - 1; j++)
            {
              poll_fds[j] = poll_fds[j + 1];
            }

            num_clients--;
          }
          else
          {
            printf("S-a primit de la clientul de pe socketul %d mesajul: %s\n",
                   poll_fds[i].fd, received_packet.message);
            /* TODO 2.1: Trimite mesajul catre toti ceilalti clienti */
            for (int j = 1; j < num_clients; j++)
            {
              if (poll_fds[j].fd != poll_fds[i].fd)
              {
                rc = send_all(poll_fds[j].fd, &received_packet,
                              sizeof(received_packet));
                DIE(rc < 0, "send");
              }
            }
          }
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cout << "\n Usage: " << argv[0] << " <port>\n";
    return 1;
  }

  // Parsam port-ul ca un numar
  uint16_t port;
  int rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  std::cout << "Port: " << port << "\n";

  /*
          TCP
  */

  // Obtinem un socket TCP pentru receptionarea conexiunilor
  int listen_tcp = socket(AF_INET, SOCK_STREAM, 0);
  DIE(listen_tcp < 0, "tcp socket");

  // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
  // rulam de 2 ori rapid
  int enable = 1;
  if (setsockopt(listen_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

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

  enable = 1;
  if (setsockopt(listen_udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

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

  int ret;
  while (1)
  {
    ret = poll(poll_fds, num_clients, -1);
    DIE(ret < 0, "eroare la poll");

    for (int i = 0; i < num_clients; i++)
    {

      // Eveniment pe stdin
      if (poll_fds[0].revents & POLLIN)
      {
        char buf[MSG_MAXSIZE];
        std::cin.getline(buf, sizeof(buf));
        if (strcmp(buf, "exit") == 0)
        {
          std::cout << "Serverul se inchide\n";

          // Inchidem socketii
          for (int nr_socket = 1; nr_socket < num_clients; nr_socket++)
          {
            close(poll_fds[nr_socket].fd);
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

      // Eveniment pe socketii TCP
      if (i > 1 && poll_fds[i].revents & POLLIN)
      {
        // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
        // pe care serverul o accepta
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int newsockfd =
            accept(poll_fds[i].fd, (struct sockaddr *)&cli_addr, &cli_len);
        DIE(newsockfd < 0, "accept");

        // se adauga noul socket intors de accept() la multimea descriptorilor
        // de citire
        poll_fds[num_clients].fd = newsockfd;
        poll_fds[num_clients].events = POLLIN;
        num_clients++;

        printf("Noua conexiune de la %s, port %d, socket client %d\n",
               inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port),
               newsockfd);
      }
    }
  }

  return 0;
}
