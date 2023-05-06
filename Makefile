CFLAGS = -Wall -g -Werror -Wno-error=unused-variable g++

COMMON_FILES = common.cpp

all: server subscriber

# Compileaza server.cpp si adauga obiectul creat pentru common.cpp
server: server.cpp common.cpp
	g++ -c common.cpp
	g++ -o server server.cpp common.cpp -Wall

# Compileaza client.cpp si adauga obiectul creat pentru common.cpp
subscriber: subscriber.cpp $(COMMON_FILES:.cpp=.o)
	g++ -o subscriber subscriber.cpp $(COMMON_FILES:.cpp=.o)

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server

# Ruleaza clientul 	
run_client:
	./subscriber

clean:
	rm -rf server subscriber common *.o *.dSYM

# Regula implicita pentru compilarea fisierelor sursa .cpp in obiecte .o
%.o: %.cpp
	$(CFLAGS) -c $<