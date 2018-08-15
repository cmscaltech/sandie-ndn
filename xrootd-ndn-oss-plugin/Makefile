CONSUMER = xrdndn-consumer
PRODUCER = xrdndn-producer

CXX	= g++ -std=c++11
CXXFLAG	= -g -Wall -O0
LDLIBS	= -lboost_system -lboost_thread-mt -lndn-cxx

SRC_DIR     = ./src
BIN_DIR     = ./bin

NDN_CONSUMER_FILES=$(wildcard $(SRC_DIR)/$(CONSUMER)*.cc)
NDN_PRODUCER_FILES=$(wildcard $(SRC_DIR)/$(PRODUCER)*.cc)

.PHONY:   all
.DEFAULT: all

all: $(CONSUMER) $(PRODUCER)

$(CONSUMER):
	$(CXX) $(CXXFLAG) $(NDN_CONSUMER_FILES) -o $(BIN_DIR)/$(CONSUMER) $(LDLIBS)

$(PRODUCER):
	$(CXX) $(CXXFLAG) $(NDN_PRODUCER_FILES) -o $(BIN_DIR)/$(PRODUCER) $(LDLIBS)

clean:
	rm -rf $(BIN_DIR)/*
