#include <iostream>
#include <unistd.h>

#include "face/face.hpp"

using namespace std;

int main(int, char**) {
    std::cout << "Begin\n";

    std::unique_ptr<ndnc::Face> face = std::make_unique<ndnc::Face>();
    face->advertise("/ndn/xrootd");

    while (true) {
        face->loop();
    }

    sleep(2);
    return 0;
}