#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <map>
#include <thread>
#include <chrono>
#include <atomic>

#include <nlohmann/json.hpp>

#include "schedulers/scheduler.h"
#include "utils/logger.h"
#include "utils/time_util.h"

Scheduler::Scheduler(){
    int port = 8080;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
}

std::string Scheduler::generateScheduleId(int conn_id){
    return "#" + std::to_string(conn_id);   
}

std::string Scheduler::generateScheduleName(std::string schedule_id){
    return "Schedule " + schedule_id;   
}

std::string Scheduler::addSchedule(nlohmann::json reqBody){
    std::string backup_type = reqBody["type"];
    std::string schedule_string = reqBody["payload"];
    std::string schedule_id = generateScheduleId(conn_id);
    std::string schedule_name = generateScheduleName(schedule_id);

    nlohmann::json taskContext;
    taskContext["backup_type"] = backup_type;
    taskContext["schedule_string"] = schedule_string;
    taskContext["schedule_id"] = schedule_id;
    taskContext["schedule_name"] = schedule_name;

    cron.add_schedule(generateScheduleName(generateScheduleId(conn_id)), reqBody["payload"], [taskContext = taskContext](auto&) {
        std::string timestamp = TimeUtil::GetCurrentTimestamp();
        
        // Add backup logic here
        // std::filesystem::path src = "~/Desktop/HPE/Source";
        // std::filesystem::path dst = "~/Desktop/HPE/Target";
        // size_t s = 1024 * 1024;
        // std::string remark = "";

        // Backup backup(src,dst, BackupType::FULL, remark,s);
        // backup.backup_directory();

        std::string message;
        message += taskContext["schedule_name"].get<std::string>() + " at " + timestamp + "\n";
        message += "-- Backup Type: " + taskContext["backup_type"].get<std::string>() + "\n" ;
        message += "-- Backup Schedule: " + taskContext["schedule_string"].get<std::string>() + "\n" ;
        
        std::cout << message << "\n";
    }); 

    nlohmann::json metaData;
    metaData["schedule"] = std::string(reqBody["payload"]);
    metaData["type"] = std::string(reqBody["type"]);

    schedules[schedule_id] = metaData.dump();
    std::string message = schedule_name + " added!\n";
    conn_id = conn_id + 1;   
    return message;
}

std::string Scheduler::viewSchedules(){
    std::string message;
    auto count = schedules.size();

    if (count == 0){
        message += "No schedules set!\n";
        return message;
    }

    for (auto p: schedules){
        count--;
        std::string id = p.first;
        nlohmann::json metaData = nlohmann::json::parse(p.second);
        
        std::string schedule = metaData["schedule"].get<std::string>();
        std::string type = metaData["type"].get<std::string>();

        message += generateScheduleName(id) + "\n";
        message += "-- Backup Type: " + type + "\n" ;
        message += "-- Backup Schedule: " + schedule + "\n" ;

        if (count != 0){
            message += "\n";
        }
    }

    return message;
}

std::string Scheduler::removeSchedule(nlohmann::json reqBody){
    std::string schedule_id = reqBody["payload"].get<std::string>();
    std::string name = generateScheduleName(schedule_id);
    
    cron.remove_schedule(name);
    schedules.erase(schedule_id);

    return name + " removed!\n";
}

void Scheduler::Run(){
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

    std::cout << "Scheduler server active.\n";
    
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
    // std::cout << "Scheduler socket closed!\n";

    // Stop the scheduler
    scheduler_thread.join(); 
    std::cout << "Shutting down scheduler server...\n";
    
    return;
} 