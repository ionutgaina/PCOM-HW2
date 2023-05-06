#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <cstdio>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Dimensiunea maxima a mesajului */
#define MSG_MAXSIZE 1500
#define TOPIC_MAXSIZE 50
#define MAX_CLIENTS 10
#define ID_MAXSIZE 10

#define YOLO_TYPE 0
#define SUBSCRIBE_TYPE 1
#define SUBSCRIBE_RESPONSE_TYPE 2

struct packet
{
  uint16_t len;
  char message[BUFSIZ];
  // 0 - a simple text (YOLO_TYPE)
  // 1 - a subscribe_packet struct (SUBSCRIBE_TYPE)
  // 2 - a subscription response  (SUBSCRIBE_RESPONSE_TYPE)
  uint8_t message_type;
};

struct subscribe_response_packet
{
  char topic[TOPIC_MAXSIZE + 1];

  // 0 - Octet de semn* urmat de un uint32_t formatat conform network byte order
  // 1 - uint16_t reprezentând modulul numărului ı̂nmultit cu 100
  // 2 - Un octet de semn, urmat de un uint32_t (ı̂n network byte order) reprezentând
  //     modulul numărului obtinut din alipirea părtii ı̂ntregi de partea zecimală a numărului,
  //     urmat de un uint8_t ce reprezintă modulul puterii negative a lui 10 cu care trebuie
  //     ı̂nmultit modulul pentru a obtine numărul original (ı̂n modul)
  // 3 - Sir de maxim 1500 de caractere, terminat cu \0 sau delimitat de finalul datagramei pentru lungimi mai mici
  uint8_t data_type;
  char content[MSG_MAXSIZE + 1];
};

struct subscribe_packet
{
  char command[11];
  char topic[TOPIC_MAXSIZE + 1];
  uint8_t sf;
};

#endif
