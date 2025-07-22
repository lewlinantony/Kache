#include "KacheServer.hpp"
#include <vector>
#include <unistd.h>
#include <iostream>
#include <cstring>

// --- Forward Declarations ---
bool read_bytes(int fd, void* buf, size_t n);
std::string read_line(int fd);


KacheServer::KacheServer(int port) : 
    port_(port), 
    pool_(std::thread::hardware_concurrency())
{
    setup_server();
}

/**
 * @brief Reads exactly 'n' bytes from a file descriptor into a buffer.
 * This is a crucial helper for TCP, as a single read() call is not
 * guaranteed to return all the requested data.
 * @param fd The file descriptor to read from.
 * @param buf The buffer to read data into.
 * @param n The number of bytes to read.
 * @return True on success, false on failure (e.g., client disconnected).
 */
bool read_bytes(int fd, void* buf, size_t n) {
    char* buf_ptr = static_cast<char*>(buf);
    size_t bytes_read = 0;
    while (bytes_read < n) {
        ssize_t result = read(fd, buf_ptr + bytes_read, n - bytes_read);
        if (result <= 0) {
            // Error or client closed connection
            return false;
        }
        bytes_read += result;
    }
    return true;
}

/**
 * @brief Reads a line from a socket, ending in \r\n.
 */
std::string read_line(int fd) {
    std::string line;
    char c;
    while (true) {
        if (!read_bytes(fd, &c, 1)) {
            return ""; // Client disconnected
        }
        if (c == '\r') {
            // Peek at the next character to see if it's '\n'
            char next_c;
            if (recv(fd, &next_c, 1, MSG_PEEK) > 0 && next_c == '\n') {
                // It is, so consume it
                read_bytes(fd, &c, 1);
            }
            break; // End of line
        }
        if (c == '\n'){ // Also handle simple newline for manual testing
            break;
        }
        line += c;
    }
    return line;
}

void KacheServer::setup_server() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        exit(EXIT_FAILURE);
    } 

    if (listen(server_fd_, 128) < 0){
        std::cerr << "Failed to listen on socket" << std::endl;
        exit(EXIT_FAILURE);
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
        
        pool_.enqueue([this, client_fd] {
            this->handle_client(client_fd, this->store_);
        });
    }
    close(server_fd_);    
}

void KacheServer::handle_client(int client_fd, KacheStore& store) {
    // Add logging to see which thread is handling which client
    // std::cerr << "[Thread " << std::this_thread::get_id() << "] Handling client fd " << client_fd << std::endl;

    while (true){
        std::string first_line = read_line(client_fd);
        
        if (first_line.empty()) {
            // std::cerr << "[fd:" << client_fd << "] Client disconnected." << std::endl;
            break;
        }

        if (first_line[0] != '*') {
            const char* err = "-ERR protocol error: expected *\r\n";
            write(client_fd, err, strlen(err));
            continue;
        }

        try {
            int num_args = std::stoi(first_line.substr(1));
            if (num_args <= 0) {
                continue;
            }

            std::vector<std::string> tokens;
            tokens.reserve(num_args);

            for (int i = 0; i < num_args; ++i) {
                std::string arg_len_line = read_line(client_fd);
                if (arg_len_line.empty() || arg_len_line[0] != '$') {
                    throw std::runtime_error("Protocol error: expected $");
                }
                int arg_len = std::stoi(arg_len_line.substr(1));

                if (arg_len < 0) { // Handle null bulk strings from client
                    tokens.push_back("");
                    continue;
                }

                std::vector<char> arg_buffer(arg_len);
                if (!read_bytes(client_fd, arg_buffer.data(), arg_len)) {
                    throw std::runtime_error("Failed to read argument bytes");
                }
                
                read_line(client_fd); // Consume the trailing \r\n

                tokens.push_back(std::string(arg_buffer.begin(), arg_buffer.end()));
            }

            if (tokens.empty()){
                continue;
            }
            
            // std::cerr << "[fd:" << client_fd << "] Received command: " << tokens[0] << std::endl;

            const std::string& cmd = tokens[0];
            std::string response;

            if ((cmd == "SET" || cmd == "set") && tokens.size() == 3){
                store.set(tokens[1], tokens[2]);
                response = "+OK\r\n";
            }
            else if ((cmd == "GET" || cmd == "get") && tokens.size() == 2){
                auto value = store.get(tokens[1]);
                if (value){
                    response = "$" + std::to_string(value->length()) + "\r\n" + *value + "\r\n";
                }
                else{
                    response = "$-1\r\n";
                }
            }
            else if ((cmd == "DEL" || cmd == "delete") && tokens.size() >= 2){
                int deleted_count = 0;
                for(size_t i = 1; i < tokens.size(); ++i) {
                    if (store.del(tokens[i])) {
                        deleted_count++;
                    }
                }
                response = ":" + std::to_string(deleted_count) + "\r\n";
            }
            else if ((cmd == "EXISTS" || cmd == "exists") && tokens.size() == 2) {
                if (store.exists(tokens[1])) {
                    response = ":1\r\n";
                } else {
                    response = ":0\r\n";
                }
            }
            else if (cmd == "PING" || cmd == "ping") {
                response = "+PONG\r\n";
            }
            else if (cmd == "COMMAND" || cmd == "command") {
                // redis-benchmark sends this. Just responding OK is fine.
                response = "+OK\r\n";
            }
            else {
                response = "-ERR unknown command '" + cmd + "'\r\n";
            }                     
            
            write(client_fd, response.c_str(), response.length());
        } catch (const std::exception& e) {
            // std::cerr << "[fd:" << client_fd << "] Exception: " << e.what() << ". Closing connection." << std::endl;
            break; 
        }
    }
    close(client_fd);    
}

void KacheServer::run() {
    accept_connections();
}
