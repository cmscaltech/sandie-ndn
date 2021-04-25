#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "face/face.hpp"
#include "ping-client.hpp"
#include "ping-server.hpp"

#include <string.h>

using namespace std;
using namespace ndnc;

bool loop = false;

void handler(sig_atomic_t signal) {
    cout << "Handle signal " << signal << "\n";
    loop = false;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Specify type of application: client/server\n";
        return -1;
    }

    signal(SIGINT, handler);
    signal(SIGABRT, handler);

    ndnc::Face *face = new ndnc::Face();
    if (!face->isValid()) {
        std::cout << "ERROR: Could not create face obj\n";
        return -1;
    }

    if (strcmp(argv[1], "client") == 0) {
        ndnc::PingClient *client = new ndnc::PingClient(*face);
        loop = true;

        while (loop) {
            sleep(1);
            client->sendInterest("/ndn/xrootd/");
            face->loop();
        }

        if (NULL != client) {
            delete client;
        }

    } else if (strcmp(argv[1], "server") == 0) {
        ndnc::PingServer *server = new ndnc::PingServer(*face);
        face->advertise("/ndn/xrootd");

        loop = true;
        while (loop) {
            usleep(5000);
            face->loop();
        }

        if (NULL != server) {
            delete server;
        }
    } else {
        std::cout << "Available options: client/server\n";
        return -1;
    }

    if (NULL != face) {
        delete face;
    }

    return 0;
}
