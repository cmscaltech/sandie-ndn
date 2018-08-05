TEST_NDN_CONSUMER = test-ndn-consumer
TEST_NDN_PRODUCER = test-ndn-producer

CXX	= g++ -std=c++11
CXXFLAG	= -g -Wall -O0
LDLIBS	= -lboost_system -lndn-cxx

SRC_DIR     = ./src
BIN_DIR     = ./bin

SRC_LIST = $(wildcard $(SRC_DIR)/*.cpp)

.PHONY:   all
.DEFAULT: all

all: $(TEST_NDN_CONSUMER) $(TEST_NDN_PRODUCER)

$(TEST_NDN_CONSUMER):
	$(CXX) $(CXXFLAG) $(wildcard $(SRC_DIR)/$(TEST_NDN_CONSUMER)*.cpp) -o $(BIN_DIR)/$(TEST_NDN_CONSUMER) $(LDLIBS)

$(TEST_NDN_PRODUCER):
	$(CXX) $(CXXFLAG) $(wildcard $(SRC_DIR)/$(TEST_NDN_PRODUCER)*.cpp) -o $(BIN_DIR)/$(TEST_NDN_PRODUCER) $(LDLIBS)

clean:
	rm -f $(BIN_DIR)/*
