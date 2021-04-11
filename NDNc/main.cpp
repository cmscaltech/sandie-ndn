#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "face/face.hpp"
#include "ping-server.hpp"

using namespace std;

bool loop = false;

void handler(sig_atomic_t signal) {
    cout << "Handle signal " << signal << "\n";
    loop = false;
}


int main(int, char**) {
    signal(SIGINT, handler);
    signal(SIGABRT, handler);

    std::cout << "Begin\n";

    std::shared_ptr<ndnc::Face> face         = std::make_shared<ndnc::Face>();
    std::shared_ptr<ndnc::PingServer> server = std::make_shared<ndnc::PingServer>(face);

    face->advertise("/ndn/xrootd");

    loop = true;
    while (loop) {
        usleep(5000);
        face->loop();
    }

    return 0;
}