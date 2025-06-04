#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>

#include <nlohmann/json.hpp>

#include "services/scheduler_service.h"

SchedulerService::SchedulerService(){
    int port = 8080;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
}

void SchedulerService::Log(){
    std::cout << "Logging not implemented yet!\n";
}


void SchedulerService::sendRequest(const char *message){
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
    std::cout << "\n";
    
    close(client_fd);
    return;
}

void SchedulerService::addSchedule(){
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

    sendRequest(reqBody.dump().c_str());
}

void SchedulerService::removeSchedule(){
    // Remove a schedule
    int schedule_id;
    std::cout << "Enter id of schedule: ";
    std::cin >> schedule_id;
    std::cin.ignore();
    
    nlohmann::json reqBody;
    reqBody["action"] = "remove";
    reqBody["payload"] = schedule_id;
    
    sendRequest(reqBody.dump().c_str());
}

void SchedulerService::viewSchedules(){
     // View all schedules
            
    nlohmann::json reqBody;
    reqBody["action"] = "view";

    sendRequest(reqBody.dump().c_str());
}

void SchedulerService::terminateScheduler(){
    // Making sure server shuts down
    char confirm;
    std::cout << "Enter Y to confirm termination of scheduler\n";
    std::cin >> confirm; 
    std::cin.ignore();

    if (confirm == 'Y' || confirm == 'y'){
        nlohmann::json reqBody;
        reqBody["action"] = "exit";
        sendRequest(reqBody.dump().c_str());
    }
}

void SchedulerService::showMainMenu(){
    while (true){
        std::cout << "Enter 1 to view all schedules\n";
        std::cout << "Enter 2 to add a schedule\n";
        std::cout << "Enter 3 to remove a schedule\n";
        std::cout << "Enter 4 to edit a schedule\n";
        std::cout << "Enter 5 to terminate scheduler\n";
        std::cout << "Enter 6 to exit\n";

        int main_menu_input;
        std::cin >> main_menu_input;
        std::cin.ignore();
        std::cout << "\n";
    
    }
}

void SchedulerService::Run(){
    // Main user loop
    while (true){
        std::cout << "Enter 1 to view all schedules\n";
        std::cout << "Enter 2 to add a schedule\n";
        std::cout << "Enter 3 to remove a schedule\n";
        std::cout << "Enter 4 to edit a schedule\n";
        std::cout << "Enter 5 to terminate scheduler\n";
        std::cout << "Enter 6 to exit\n";

        int main_menu_input;
        std::cin >> main_menu_input;
        std::cin.ignore();
        std::cout << "\n";

        if (main_menu_input == 1){
            viewSchedules();
        }

        else if (main_menu_input == 2){
            addSchedule();
        }

        else if (main_menu_input == 3){
            removeSchedule();
        }

        else if (main_menu_input == 4){
            // To do: Edit cron jobs?
            break;
        }
        
        else if (main_menu_input == 5){
            terminateScheduler();
            break;
        }

        else{
            break;
        }
    }


}