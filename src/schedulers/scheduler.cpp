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

#include "schedulers/scheduler.h"
#include "schedulers/schedule.h"
#include "utils/logger.h"
#include "utils/time_util.h"
#include "backup_restore/backup.hpp"

Scheduler::Scheduler(){
    int port = 8080;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
}

std::string Scheduler::GenerateScheduleId(int conn_id){
    return "#" + std::to_string(conn_id);   
}

std::string Scheduler::GenerateScheduleName(std::string schedule_id){
    return "Schedule " + schedule_id;   
}

std::string Scheduler::GenerateScheduleInfoString(std::string schedule_id){
    std::string message;
    
    nlohmann::json metaData = nlohmann::json::parse(schedules[schedule_id]);
    std::string type = metaData["type"].get<std::string>();
    std::string schedule = metaData["schedule"].get<std::string>();
    std::string source = metaData["source"].get<std::string>();
    std::string destination = metaData["destination"].get<std::string>();
    std::string remarks = metaData["remarks"].get<std::string>();

    message += GenerateScheduleName(schedule_id) + "\n";
    message += "-- Backup Type: " + type + "\n" ;
    message += "-- Backup Schedule: " + schedule + "\n";
    message += "-- Source: " + source + "\n";
    message += "-- Destination: " + destination + "\n";
    message += "-- Remarks: " + remarks + "\n";
    return message;
}

std::string Scheduler::AddSchedule(nlohmann::json reqBody){
    std::vector <std::string> v = {"Full","Incremental","Differential"};

    int backup_type = reqBody["type"];
    std::string backup_type_string = v[backup_type];
    std::string source = reqBody["source"];
    std::string destination = reqBody["destination"];
    std::string remarks = reqBody["remarks"];
    std::string schedule_string = reqBody["payload"];
    std::string schedule_id = GenerateScheduleId(conn_id);
    std::string schedule_name = GenerateScheduleName(schedule_id);
    
    // Stored inside the map as metadata
    nlohmann::json metaData;
    metaData["type"] = backup_type_string;
    metaData["schedule"] = schedule_string;
    metaData["source"] = source;
    metaData["destination"] = destination;
    metaData["remarks"] = remarks;
    
    schedules[schedule_id] = metaData.dump();
    conn_id = conn_id + 1;
    
    // This is tied to scheduled function
    nlohmann::json taskContext;
    taskContext["source"] = source;
    taskContext["destination"] = destination;
    taskContext["backup_type"] = backup_type;
    taskContext["remarks"] = remarks;
    taskContext["info"] = GenerateScheduleInfoString(schedule_id);
    
    // Adding the scheduled function
    // cron.add_schedule(GenerateScheduleName(GenerateScheduleId(conn_id)), reqBody["payload"], [taskContext = taskContext](auto&) {
    //     std::string timestamp = TimeUtil::GetCurrentTimestamp();
    //     std::cout << "Executed At " << timestamp << "\n";
    //     std::cout << taskContext["info"].get<std::string>() << "\n";
        
    //     Backup backup(taskContext["source"], taskContext["backup_type"], taskContext["remarks"]);
    //     backup.BackupDirectory();

    //     // Pending nonlocal backups
    // }); 
    
    return schedule_id;
}

std::string Scheduler::ViewSchedules(){
    nlohmann::json res;
    nlohmann::json schedule_array = nlohmann::json::array();
    
    for (auto p: schedules){
        nlohmann::json metaData = nlohmann::json::parse(p.second);
        nlohmann::json schedule_json;
        schedule_json["name"] = p.first;
        schedule_json["schedule"] = metaData["schedule"].get<std::string>();
        schedule_json["source"] = metaData["source"].get<std::string>();
        schedule_json["destination"] = metaData["destination"].get<std::string>();
        schedule_json["remarks"] = metaData["remarks"].get<std::string>();
        schedule_json["backup_type"] = metaData["type"].get<std::string>();
    
        schedule_array.push_back(schedule_json);
    }
    
    res["schedules"] = schedule_array;
    return res.dump();
}

std::string Scheduler::RemoveSchedule(nlohmann::json reqBody){
    std::string schedule_id = reqBody["payload"].get<std::string>();
    std::string name = GenerateScheduleName(schedule_id);
    
    cron.remove_schedule(name);
    schedules.erase(schedule_id);

    return schedule_id;
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
            message = AddSchedule(reqBody);
        }

        else if (reqBody["action"] == "view"){
            message = ViewSchedules();
        }

        else if (reqBody["action"] == "remove"){
            message = RemoveSchedule(reqBody);
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

    // Stop the scheduler
    scheduler_thread.join(); 
    std::cout << "Shutting down scheduler server...\n";
    
    return;
} 