#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>

#include <nlohmann/json.hpp>

#include <schedulers/client.h>

Client::Client(){
    // Defining address specifics
    int port = 8080;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
}

void Client::Run(){
    // Main user loop
    while (true){
        std::cout << "Enter 1 to view all schedules\n";
        std::cout << "Enter 2 to add a schedule\n";
        std::cout << "Enter 3 to remove a schedule\n";
        std::cout << "Enter 4 to edit a schedule\n";
        std::cout << "Enter 5 to exit\n";

        int main_menu_input;
        std::cin >> main_menu_input;
        std::cin.ignore();
        std::cout << "\n";

        if (main_menu_input == 1){
            // View all schedules
            
            nlohmann::json reqBody;
            reqBody["action"] = "view";

            Send(reqBody.dump().c_str());
        }

        else if (main_menu_input == 2){
            // Add a schedule

            std::string schedule;
            std::string type;

            std::cout << "Enter schedule to send: ";
            std::getline(std::cin, schedule);
            std::cout << "Enter type of backup: ";
            std::getline(std::cin, type);
            std::cout << "\n";
            
            nlohmann::json reqBody;
            reqBody["action"] = "add";
            reqBody["payload"] = schedule;
            reqBody["type"] = type;

            Send(reqBody.dump().c_str());

        }

        else if (main_menu_input == 3){
            // Remove a schedule

            int schedule_id;
            std::cout << "Enter id of schedule: ";
            std::cin >> schedule_id;
            std::cin.ignore();
            
            nlohmann::json reqBody;
            reqBody["action"] = "remove";
            reqBody["payload"] = schedule_id;
            
            Send(reqBody.dump().c_str());
        }

        else if (main_menu_input == 4){
            // To do: Edit cron jobs?
            break;
        }
        
        else {
            break;
        }
    }

    // Making sure server shuts down
    nlohmann::json reqBody;
    reqBody["action"] = "exit";
    Send(reqBody.dump().c_str());
}

void Client::Send(const char *message){
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        std::cout << "Socket failed!\n";
        return;
    }

    // Convert IPv4 and connect
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address or Address not supported\n";
        return;
    }

    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection failed!\n";
        return;
    }

    send(client_fd, message, strlen(message), 0);

    // Fetch server response and display
    char server_msg[1024];
    if (recv(client_fd, server_msg, sizeof(server_msg), 0 ) < 0){
        std::cout << "Error\n";
        return;
    }

    std::cout << server_msg << "\n";
    
    close(client_fd);
    return;
}
