GOFLAGS := -a

all: consumer

consumer:
	go install $(GOFLAGS) ./cmd/sandie-consumer

.PHONY: all clean
clean:
	go clean -cache -testcache
	rm ${GOPATH}/bin/sandie-consumer
