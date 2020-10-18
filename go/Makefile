GOFLAGS := -a

all: consumer

consumer:
	go install $(GOFLAGS) ./cmd/sandie-consumer

sandie:
	go install $(GOFLAGS) ./cmd/sandie

godeps:
	go get -u ./...
	go get -d ./...

.PHONY: all clean
clean:
	go clean -cache -testcache
	rm ${GOPATH}/bin/sandie-consumer
	rm ${GOPATH}/bin/sandie
