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
    topic_to_messages[topic] = std::vector<std::string>();
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

  bool has_topic(std::string topic)
  {
    return topic_to_sf.find(topic) != topic_to_sf.end();
  }

  void add_message(std::string topic, std::string message)
  {
    topic_to_messages[topic].push_back(message);
  }

  std::vector<std::string> get_messages(std::string topic)
  {
    std::vector<std::string> messages = topic_to_messages[topic];
    topic_to_messages[topic].clear();
    return messages;
  }

  std::vector<std::string> get_topics_sf_1()
  {
    std::vector<std::string> topics;
    for (auto it = topic_to_sf.begin(); it != topic_to_sf.end(); ++it)
    {
      if (it->second == 1)
      {
        topics.push_back(it->first);
      }
    }
    return topics;
  }

  std::vector<std::string> get_messages_from_topics(std::vector<std::string> topics)
  {
    std::vector<std::string> messages;
    for (auto it = topics.begin(); it != topics.end(); ++it)
    {
      std::vector<std::string> messages_from_topic = get_messages(*it);
      messages.insert(messages.end(), messages_from_topic.begin(), messages_from_topic.end());
    }
    return messages;
  }

  void cout_topics_and_sf(std::string id)
  {
    std::cout << "TOPICS AND SF: pt user-ul :" << id << "\n";
    for (auto it = topic_to_sf.begin(); it != topic_to_sf.end(); ++it)
    {
      std::cout << it->first << " " << it->second << "\n";
    }
  }
};