#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Dimensiunea maxima a mesajului */
#define MSG_MAXSIZE 1500
#define TOPIC_MAXSIZE 50
#define MAX_CLIENTS 10

struct chat_packet {
  uint16_t len;
  char message[MSG_MAXSIZE + 1];
};

struct topic {
  char name[TOPIC_MAXSIZE + 1];
  int clients[MAX_CLIENTS];
  int clients_count;
};

#endif
