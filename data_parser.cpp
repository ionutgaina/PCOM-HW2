#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include "common.h"

class Data_Parser
{
public:
  std::string parse_udp_message(udp_packet message, sockaddr_in udp_client)
  {
    std::stringstream resultstream;
    std::string ip = inet_ntoa(udp_client.sin_addr);
    uint16_t port = ntohs(udp_client.sin_port);

    std::string topic = message.topic;

    resultstream << ip << ":" << port << " - " << topic << " - ";

    switch (message.data_type)
    {
    case 0:
    {
      resultstream << "INT - ";

      uint8_t sign = message.content[0];
      uint32_t number = ntohl(*(uint32_t *)(message.content + 1));

      sign == 1 ? resultstream << "-" : resultstream << "";
      resultstream << number;
      break;
    }

    case 1:
    {
      resultstream << "SHORT_REAL - ";

      uint16_t number = ntohs(*(uint16_t *)(message.content));
      double real = static_cast<double>(number) / 100.0;
      resultstream << std::fixed << std::setprecision(2) << real;
      break;
    }

    case 2:
    {
      resultstream << "FLOAT - ";

      uint8_t sign = message.content[0];
      uint32_t number = ntohl(*(uint32_t *)(message.content + 1));
      uint8_t power = message.content[5];

      sign == 1 ? resultstream << "-" : resultstream << "";

      float number_float = static_cast<float>(number);

      for (int i = 0; i < power; i++)
      {
        number_float /= 10.0;
      }

      resultstream << std::fixed << std::setprecision(power) << number_float;
      break;
    }

    case 3:
    {
      resultstream << "STRING - ";

      resultstream << message.content;
      break;
    }
    default:
    {
      return "";
      break;
    }
    }

    return resultstream.str();
  };
};