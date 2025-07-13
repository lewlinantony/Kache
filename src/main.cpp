#include <iostream>
#include "KacheStore.hpp"
#include <sys/socket.h> // For socket(), bind(), etc.
#include <netinet/in.h> // For sockaddr_in
#include <unistd.h>     // For close()
#include <thread>

const int PORT = 6379;

void handle_client(int client_fd, KacheStore &store){
    char buffer[1024] = {0};
    std::string command;
    
    if (read(client_fd, buffer, 1024) <= 1){
        std::cout << "No Request Message detected" << std::endl;
    }
    else{
        command = std::string(buffer);
        command.erase(command.find_last_not_of("\r\n") + 1); // to remove \n from the end
        std::cout << "Request Message >" << command << std::endl; 
    }

    std::string response;
    if (command == "PING"){
        response = "PONG\n";    
    }
    else{
        response = "WONG\n";
    }
    write(client_fd, response.c_str(), response.length());
    close(client_fd);
}

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

    KacheStore store;

    while (true){
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0){
            std::cerr << "Failed to accept Connection" << std::endl;
            continue;
        }
        
        std::cout << "New client Connected" << std::endl;

        std::thread worker(handle_client, client_fd, std::ref(store));
        worker.detach();
    }

    close(server_fd);

    
    return 0;
}