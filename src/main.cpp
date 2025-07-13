#include <iostream>
#include "KacheStore.hpp"
#include <sys/socket.h> // For socket(), bind(), etc.
#include <netinet/in.h> // For sockaddr_in
#include <unistd.h>     // For close()

const int PORT = 6379;

int main() {
    // server file descriptor
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl; //using cerr we can choose to print or not print the cerr outputs using its file descriptors
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        std::cerr << "Failed to bind to port " << PORT << std::endl;
        return 1;
    } 

    if (listen(server_fd, 10) < 0){
        std::cerr << "Failed to listen on socker" << std::endl;
        return 1;
    }

    std::cout << "Kache server listening on port " << PORT << std::endl;


    while (true){
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0){
            std::cerr << "Failed to accept Connection" << std::endl;
            continue;
        }
        
        std::cout << "New client Connected" << std::endl;
    }

    close(server_fd);

    
    return 0;
}