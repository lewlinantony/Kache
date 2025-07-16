#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Helper to convert a command like 'SET key value' to the RESP format.
std::string command_to_resp(const std::vector<std::string>& tokens) {
    std::stringstream resp;
    resp << "*" << tokens.size() << "\r\n";
    for (const auto& token : tokens) {
        resp << "$" << token.length() << "\r\n";
        resp << token << "\r\n";
    }
    return resp.str();
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(6379); // Default Kache/Redis port

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed to Kache server on 127.0.0.1:6379" << std::endl;
        return -1;
    }

    std::cout << "Connected to Kache server. Type 'exit' or 'quit' to close." << std::endl;
    std::string line;

    // This is the Read-Eval-Print-Loop (REPL)
    while (std::cout << "kache> ", std::getline(std::cin, line)) {
        if (line == "exit" || line == "quit") {
            break;
        }

        std::stringstream ss(line);
        std::vector<std::string> tokens;
        std::string token;
        while (ss >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            continue;
        }

        // Convert the user's command to RESP and send it
        std::string resp_command = command_to_resp(tokens);
        send(sock, resp_command.c_str(), resp_command.length(), 0);

        // Read the server's response and print it
        char buffer[1024] = {0};
        int valread = read(sock, buffer, 1024);
        if (valread > 0) {
            std::cout << std::string(buffer, valread);
        } else {
            std::cout << "Server disconnected." << std::endl;
            break;
        }
    }

    close(sock);
    return 0;
}