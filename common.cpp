#include "common.h"
#include "helpers.h"

#include <sys/socket.h>
#include <sys/types.h>

int recv_allpacket(int sockfd, struct packet *recv_packet)
{
  int ret = recv_all(sockfd, &recv_packet->header, sizeof(struct packet_header));
  DIE(ret < 0, "recv_all");
  if (ret == 0)
  {
    return 0;
  }

  recv_packet->content = new char[recv_packet->header.len];

  ret = recv_all(sockfd, recv_packet->content, recv_packet->header.len);
  DIE(ret < 0, "recv_all");
  return ret;
}

int recv_all(int sockfd, void *buffer, size_t len)
{

  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;

  while (bytes_remaining)
  {
    bytes_received = recv(sockfd, buff, bytes_remaining, 0);
    if (bytes_received <= 0)
    {
      return bytes_received;
    }
    bytes_remaining -= bytes_received;
    buff += bytes_received;
  }

  return bytes_received;
}

int send_allpacket(int sockfd, struct packet *sent_packet)
{
  int ret = send_all(sockfd, &sent_packet->header, sizeof(struct packet_header));
  DIE(ret < 0, "send_all");

  ret = send_all(sockfd, sent_packet->content, sent_packet->header.len);
  DIE(ret < 0, "send_all");
  free(sent_packet->content);
  return ret;
}

int send_all(int sockfd, void *buffer, size_t len)
{
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;

  while (bytes_remaining)
  {
    bytes_sent = send(sockfd, buff, bytes_remaining, 0);
    if (bytes_sent <= 0)
    {
      return bytes_sent;
    }
    bytes_remaining -= bytes_sent;
    buff += bytes_sent;
  }

  return bytes_sent;
}