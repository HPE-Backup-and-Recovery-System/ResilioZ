#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "services/scheduler_service.h"
#include "backup_restore/backup.hpp"
#include "utils/error_util.h"
#include "utils/logger.h"
#include "utils/prompter.h"
#include "utils/user_io.h"

SchedulerService::SchedulerService(){
    int port = 8080;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
}

void SchedulerService::Log(){
    Logger::TerminalLog("Scheduler service is running...");
}


void SchedulerService::SendRequest(const char *message){
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        ErrorUtil::ThrowError("Socket creation failed");
    }

    // Convert IPv4 and connect
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        ErrorUtil::ThrowError("Invalid address or address not supported!");
    }

    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        ErrorUtil::ThrowError("Connection to scheduler server failed!");
        return;
    }

    send(client_fd, message, strlen(message), 0);

    // Fetch server response and display
    char server_msg[1024];
    if (recv(client_fd, server_msg, sizeof(server_msg), 0 ) < 0){
        ErrorUtil::ThrowError("Message from scheduler server not received!");
    }

    std::cout << server_msg;
    
    close(client_fd);
    return;
}

void SchedulerService::AddSchedule(){
    UserIO::DisplayTitle("Adding Schedule");
    std::string schedule, backup_type, source, destination,remarks;
    std::vector<std::string> menu;
    int choice;
    BackupType type;

    schedule = Prompter::PromptCronString();

    source = Prompter::PromptPath("Path to Backup Source");
    destination = Prompter::PromptPath("Path to Backup Destination");
    menu = {"Full Backup", "Incremental Backup", "Differential Backup"};
    choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Backup Type", false), menu);
    type = static_cast<BackupType>(choice);
    remarks = Prompter::PromptInput("Remarks for Backup (Optional)");
    
    nlohmann::json reqBody;
    reqBody["action"] = "add";
    reqBody["payload"] = schedule;
    reqBody["source"] = source;
    reqBody["destination"] = destination;
    reqBody["type"] = type;
    reqBody["remarks"] = remarks;

    SendRequest(reqBody.dump().c_str());
}

void SchedulerService::RemoveSchedule(){
    UserIO::DisplayTitle("Removing Schedule");
    std::string schedule_id;
    
    schedule_id = Prompter::PromptScheduleId("Schedule ID");
    
    nlohmann::json reqBody;
    reqBody["action"] = "remove";
    reqBody["payload"] = schedule_id;
    
    SendRequest(reqBody.dump().c_str());
}

void SchedulerService::ViewSchedules(){
    UserIO::DisplayTitle("Viewing All Schedules");
            
    nlohmann::json reqBody;
    reqBody["action"] = "view";

    SendRequest(reqBody.dump().c_str());
}

void SchedulerService::TerminateScheduler(){
    // Making sure server shuts down
    char confirm;
    std::cout << "Enter Y to confirm termination of scheduler\n";
    std::cin >> confirm; 
    std::cin.ignore();

    if (confirm == 'Y' || confirm == 'y'){
        nlohmann::json reqBody;
        reqBody["action"] = "exit";
        SendRequest(reqBody.dump().c_str());
    }
}

void SchedulerService::ShowMainMenu(){
    std::vector<std::string> main_menu = {
      "Go BACK...", "Create New Schedule", "List All Schedules",
      "Delete Schedule"};
    
    while (true) {
        int choice = UserIO::HandleMenuWithSelect(
            UserIO::DisplayMaxTitle("Scheduler Service", false), main_menu);

        try {
        switch (choice) {
            case 0:
            std::cout << "\n - Going Back...\n";
            return;
            case 1:
            AddSchedule();
            break;
            case 2:
            ViewSchedules();
            break;
            case 3:
            RemoveSchedule();
            break;
            default:
            Logger::TerminalLog("Menu Mismatch...", LogLevel::ERROR);
        }
        } catch (const std::exception& e) {
        ErrorUtil::LogException(e, "Scheduler Service");
        }
    }
}

void SchedulerService::Run(){
    // Main user loop
    ShowMainMenu();
}