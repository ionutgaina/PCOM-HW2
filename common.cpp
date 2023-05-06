#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

int recv_all(int sockfd, void *buffer, size_t len)
{

  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;
  

      while(bytes_remaining) {
            bytes_received = recv(sockfd, buff, bytes_remaining, 0);
            if (bytes_received < 0) {
                return bytes_received;
            }
            bytes_remaining -= bytes_received;
            buff += bytes_received;
      }

  
  return recv(sockfd, buffer, len, 0);
}

int send_all(int sockfd, void *buffer, size_t len)
{
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;
  
      while(bytes_remaining) {
            bytes_sent = send(sockfd, buff, bytes_remaining, 0);
            if (bytes_sent < 0) {
                return bytes_sent;
            }
            bytes_remaining -= bytes_sent;
            buff += bytes_sent;
      }
  

  return send(sockfd, buffer, len, 0);
}