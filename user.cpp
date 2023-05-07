#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <vector>

class Client_TCP {
  public:
    std::unordered_map<std::string, int> topic_to_sf;
    int socket;

    Client_TCP(int socket) {
      this->socket = socket;
    }


    bool operator==(const Client_TCP& other) const {
      return socket == other.socket;
    }
};