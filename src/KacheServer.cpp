#include "KacheServer.hpp"

KacheServer::KacheServer(int port) : port_(port){
    setup_server();
}

std::string read_line(int fd) {
    std::string line;
    char c;

    while (read(fd, &c, 1) > 0) {
        // Stop reading at the first sign of a line break.
        if (c == '\r') {
            // This is a simplified way to handle the \r\n pair.
            char next_c;
            // The `MSG_PEEK` flag lets us look at the next byte without removing it from the stream.
            if (recv(fd, &next_c, 1, MSG_PEEK) > 0 && next_c == '\n') {
                // It is indeed a \n. Read it to consume it.
                read(fd, &c, 1);
            }
            break; // Stop reading.
        }
        if (c == '\n') {
            // Found a Unix-style newline for manual testing
            break;
        }
        line += c;
    }
    return line;
}

void KacheServer::setup_server() {
    // server file descriptor
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        std::cerr << "Failed to create socket" << std::endl; // using cerr we can choose to print or not print the cerr outputs using its file descriptors
        return;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        return;
    } 

    if (listen(server_fd_, 10) < 0){
        std::cerr << "Failed to listen on socker" << std::endl;
        return;
    }

    std::cout << "Kache server listening on port " << port_ << std::endl;    
}

void KacheServer::accept_connections() {
    while (true){
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0){
            std::cerr << "Failed to accept Connection" << std::endl;
            continue;
        }
        
        std::cout << "New client Connected" << std::endl;

        std::thread worker(handle_client, client_fd, std::ref(store_));
        worker.detach();
    }

    close(server_fd_);    
}

std::vector<std::string> parse_command(const std::string& command_str) {
    std::vector<std::string> tokens;
    std::stringstream ss(command_str);
    std::string token;

    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

void KacheServer::handle_client(int client_fd, KacheStore& store) {
    std::string command;
    
    while (true){
        std::string first_line = read_line(client_fd);
        
        // If the client disconnects, read_line will return an empty string.
        if (first_line.empty()) {
            std::cout << "Client Connection Closed" << std::endl;
            break;
        }

        // Make sure the command starts with '*' which means it's an Array of commands
        if (first_line[0] != '*') {
            write(client_fd, "ERR protocol error: expected *\n", 30);
            continue;
        }

        int num_args = std::stoi(first_line.substr(1));
        std::vector<std::string> tokens;

        // Loop for the number of arguments we expect
        for (int i = 0; i < num_args; ++i) {
            // Read the line that tells us the argument's length. e.g., "$3" for "SET"
            std::string arg_len_line = read_line(client_fd);
            if (arg_len_line.empty() || arg_len_line[0] != '$') {
                write(client_fd, "ERR protocol error: expected $\n", 30);
                break;
            }
            int arg_len = std::stoi(arg_len_line.substr(1));

            // Use a vector to safely allocate the buffer
            std::vector<char> arg_buffer(arg_len);
            read(client_fd, arg_buffer.data(), arg_len);
            
            // Read the final \r\n and throw it away.
            read_line(client_fd);

            // Add the clean argument to our tokens.
            tokens.push_back(std::string(arg_buffer.begin(), arg_buffer.end()));
        }

        std::string response;

        if (tokens.empty()){
            response = "ERROR empty command";
        }
        else{
            const std::string& cmd = tokens[0];

            if (cmd =="SET" && tokens.size() == 3){
                store.set(tokens[1], tokens[2]);
                response ="OK\n";
            }
            else if (cmd == "GET" && tokens.size() == 2){
                auto value = store.get(tokens[1]);
                if (value){
                    response = *value + "\n";
                }
                else{
                    response = "(nil)\n";
                }
            }
            else if (cmd == "DELETE" && tokens.size() == 2){
                if (store.del(tokens[1])){
                    response = "(integer) 1\n";
                }
                else{
                    response = "(integer) 0\n";
                }

            }
            else if (cmd == "EXISTS" && tokens.size() == 2) {
                if (store.exists(tokens[1])) {
                    response = "(integer) 1\n"; // Key exists
                } else {
                    response = "(integer) 0\n"; // Key does not exist
                }
            }   
            else {
                response = "ERR unknown command '" + cmd + "'\n";
            }                     
        }
        write(client_fd, response.c_str(), response.length());
    }
    close(client_fd);    
}

void KacheServer::run() {
    accept_connections();
}