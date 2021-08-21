#Makefile for file-transfer-benchmark client and server applications

CC = g++
CPPFLAGS = -std=c++2a -Wall -g -O0 -DDEBUG -D_GNU_SOURCE=1
INCLUDE = -I$(realpath .)
LDFLAGS = -lndn-cxx -lboost_system  -lboost_program_options -lboost_filesystem -lcurl -lpthread -lmemif

all: consumer producer

packet-handler.o: face/packet-handler.cpp
	$(CC) $(CPPFLAGS) -c $< $(INCLUDE)
pipeline-interests.o: face/pipeline-interests.cpp
	$(CC) $(CPPFLAGS) -c $< $(INCLUDE)
face.o: face/face.cpp
	$(CC) $(CPPFLAGS) -c $< $(INCLUDE)
gqlclient.o: mgmt/client.cpp
	$(CC) $(CPPFLAGS) -c $<

file-transfer-client.o: ./app/file-transfer-benchmark/client/file-transfer-client.cpp
	$(CC) $(CPPFLAGS) -c $< $(INCLUDE)

file-transfer-server.o: ./app/file-transfer-benchmark/server/file-transfer-server.cpp
	$(CC) $(CPPFLAGS) -c $< $(INCLUDE)
	
main-consumer.o: app/file-transfer-benchmark/client/main.cpp
	$(CC) $(CPPFLAGS) -c $< $(INCLUDE)

main-producer.o: app/file-transfer-benchmark/server/main.cpp
	$(CC) $(CPPFLAGS) -c $< $(INCLUDE)

consumer: main-consumer.o file-transfer-client.o packet-handler.o pipeline-interests.o face.o gqlclient.o
	$(CC) $(CPPFLAGS) -o consumer main.o file-transfer-client.o packet-handler.o pipeline-interests.o face.o client.o $(INCLUDE) $(LDFLAGS)

producer: main-producer.o file-transfer-server.o packet-handler.o face.o gqlclient.o
	$(CC) $(CPPFLAGS) -o producer main.o file-transfer-server.o packet-handler.o face.o client.o $(INCLUDE) $(LDFLAGS)

.PHONY: all clean

clean:
	rm -f core consumer producer main.o file-transfer-client.o file-transfer-server.o packet-handler.o pipeline-interests.o face.o client.o
