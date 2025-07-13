#include "KacheServer.hpp"

int main() {
    KacheServer server(6379);
    server.run();
    return 0;
}