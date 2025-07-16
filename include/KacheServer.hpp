#pragma once
#include <iostream>
#include <sys/socket.h> // For socket(), bind(), etc.
#include <netinet/in.h> // For sockaddr_in
#include <unistd.h>     // For close()
#include <thread>
#include <mutex>
#include "KacheStore.hpp"
#include "ThreadPool.hpp" 

class KacheServer {
public:
    KacheServer(int port);
    void run();

private:
    void setup_server();
    void accept_connections();
    static void handle_client(int client_fd, KacheStore& store);

    int port_;
    int server_fd_;
    KacheStore store_;
    ThreadPool pool_;
};