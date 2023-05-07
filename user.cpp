#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <vector>

class Client_TCP
{
public:
  std::unordered_map<std::string, int> topic_to_sf;
  std::unordered_map<std::string, std::vector<std::string>> topic_to_messages;
  int socket;

  Client_TCP(int socket)
  {
    this->socket = socket;
  }

  bool operator==(const Client_TCP &other) const
  {
    return socket == other.socket;
  }

  void subscribe_topic(std::string topic, int sf)
  {
    topic_to_sf[topic] = sf;
  }

  void unsubscribe_topic(std::string topic)
  {
    topic_to_sf.erase(topic);
    topic_to_messages.erase(topic);
  }

  bool is_subscribed(std::string topic)
  {
    return topic_to_sf.find(topic) != topic_to_sf.end();
  }

  int get_sf(std::string topic)
  {
    return topic_to_sf[topic];
  }

  void cout_topics_and_sf(std::string id){
    std::cout << "TOPICS AND SF: pt user-ul :" << id << "\n";
    for (auto it = topic_to_sf.begin(); it != topic_to_sf.end(); ++it)
    {
      std::cout << it->first << " " << it->second << "\n";
    }
  }
};