#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

#include <nlohmann/json.hpp>

#include <schedulers/server.h>

Server::Server(){
    int port = 8080;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
}

void Server::Run(){
    // Socket creation
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0){
        std::cout << "Socket failed!\n";
        return;
    }

    // To prevent address from being unavailale for a while
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        std::cout << "setsockopt failed\n";
        return;
    }

    // Socket binding
    int addrlen = sizeof(address);
    if (bind(server_fd, (struct sockaddr*)&address, addrlen) < 0) {
        std::cout << "Bind failed\n";
        return;
    }

    // Socket listening (backlog - 3)
    if (listen(server_fd, 3) < 0) {
        std::cout << "Listen failed\n";
        return;
    }

    std::atomic<bool> running(true);

    // Running loop for scheduler
    std::thread scheduler_thread([&]() {
        while(running){
            cron.tick();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    
    while (running){
        // Accept incoming connection
        int acceptor_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (acceptor_fd < 0) {
            std::cout << "Accept failed\n";
            continue;
        }
        
        // Reading client sent content
        char buffer[1024] = {0};
        
        if (read(acceptor_fd, buffer, 1024) < 0){
            std::cout << "Read failed!";
            close(acceptor_fd);
            continue;
        }

        std::string message; // Response sent back to client

        nlohmann::json reqBody = nlohmann::json::parse(buffer);
        if (reqBody["action"] == "exit"){
            message = "Shutting down...!";
        }

        else if (reqBody["action"] == "add"){
            message = addSchedule(reqBody);
        }

        else if (reqBody["action"] == "view"){
            message = viewSchedules();
        }

        else if (reqBody["action"] == "remove"){
            message = removeSchedule(reqBody);
        }

        else{
            message = "Undefined request!";
        }

        const char *c_message = message.c_str();
        send(acceptor_fd, c_message, strlen(c_message) + 1, 0);
        close(acceptor_fd);

        if (reqBody["action"] == "exit"){
            running = false;
            break;
        }
    }

    // Close the server socket
    close(server_fd);
    // std::cout << "Server socket closed!\n";

    // Stop the scheduler
    scheduler_thread.join(); 
    
    return;
} 