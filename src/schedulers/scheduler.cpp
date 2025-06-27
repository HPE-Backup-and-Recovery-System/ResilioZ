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
#include "utils/error_util.h"
#include "utils/time_util.h"
#include "backup_restore/backup.hpp"
#include "repositories/all.h"

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
    
    // Can't find metadata corresponding to schedule_id
    auto schedule_it = schedules.find(schedule_id);
    if (schedule_it == schedules.end()){
        ErrorUtil::ThrowError("Backup metadata not found for schedule: " + schedule_id);
        return "";
    }
    
    // Can't find repo_data corresponding to schedule_id
    auto repository_it = repo_data.find(schedule_id);
    if (repository_it == repo_data.end()){
        ErrorUtil::ThrowError("Repository data not found for schedule: " + schedule_id);
        return "";
    }
    
    std::string message;
    nlohmann::json metaData = nlohmann::json::parse(schedule_it->second);

    std::string type = metaData["type"].get<std::string>();
    std::string schedule = metaData["schedule"].get<std::string>();
    std::string source = metaData["source"].get<std::string>();
    std::string remarks = metaData["remarks"].get<std::string>();

    Repository *repo = repository_it->second;

    message += GenerateScheduleName(schedule_id) + "\n";
    message += "-- Backup Type: " + type + "\n" ;
    message += "-- Backup Schedule: " + schedule + "\n";
    message += "-- Source: " + source + "\n";
    message += "-- Destination: " + repo->GetRepositoryInfoString() + "\n";
    message += "-- Remarks: " + remarks + "\n";
    return message;
}

std::string Scheduler::AddSchedule(nlohmann::json reqBody){
    std::vector <std::string> v = {"Full","Incremental","Differential"};

    int backup_type = reqBody["type"];
    std::string backup_type_string = v[backup_type];
    std::string source = reqBody["source"];

    std::string destination_name = reqBody["destination_name"];
    std::string destination_path = reqBody["destination_path"];
    std::string destination_password = reqBody["destination_password"];
    std::string destination_created_at = reqBody["destination_created_at"];
    std::string destination_type = reqBody["destination_type"];

    std::string remarks = reqBody["remarks"];
    std::string schedule_string = reqBody["payload"];
    std::string schedule_id = GenerateScheduleId(conn_id);
    std::string schedule_name = GenerateScheduleName(schedule_id);

    // Storing repository
    if (destination_type == "local"){
        repo_data[schedule_id] = new LocalRepository(destination_path, destination_name,
                                 destination_password,destination_created_at);
    }

    else if (destination_type == "nfs"){
        repo_data[schedule_id] = new NFSRepository(destination_path, destination_name,
                                 destination_password,destination_created_at);
    }

    else if (destination_type == "remote"){
        repo_data[schedule_id] = new RemoteRepository(destination_path, destination_name,
                                 destination_password,destination_created_at);
    }

    else{
        ErrorUtil::ThrowError("Unknown repository type: " + destination_type);
        return "";
    }
    
    // Storing metadata
    nlohmann::json metaData;
    metaData["type"] = backup_type_string;
    metaData["schedule"] = schedule_string;
    metaData["source"] = source;
    metaData["remarks"] = remarks;
    
    schedules[schedule_id] = metaData.dump();
    conn_id = conn_id + 1;
    
    // This is tied to scheduled function
    nlohmann::json taskContext;
    taskContext["source"] = source;
    taskContext["backup_type"] = backup_type;
    taskContext["remarks"] = remarks;
    taskContext["schedule_id"] = schedule_id;
    
    // Adding the scheduled function
    cron.add_schedule(schedule_name, schedule_string, [this, taskContext = taskContext](auto&) {
        std::string schedule_id_ = taskContext["schedule_id"].get<std::string>();
        
        // Check repository exists
        auto it = repo_data.find(schedule_id_);
        if (it == repo_data.end()) {
            return;
        }
        
        Repository *repo = it->second;
        std::string timestamp = TimeUtil::GetCurrentTimestamp();

        Backup backup(repo, taskContext["source"], taskContext["backup_type"], taskContext["remarks"]);
        backup.BackupDirectory();
        
        Logger::Log("Backup success: " + timestamp);
        Logger::Log(GenerateScheduleInfoString(schedule_id_));

    }); 
    
    return schedule_id;
}

std::string Scheduler::ViewSchedules(){
    nlohmann::json res;
    nlohmann::json schedule_array = nlohmann::json::array();

    if (schedules.size() != repo_data.size()){
        ErrorUtil::ThrowError("Mismatch between scheduler metadata and repodata");
        return "";
    }
    
    for (auto p: schedules){
        nlohmann::json metaData = nlohmann::json::parse(p.second);
        
        // Check existence of repository
        auto repository_it = repo_data.find(p.first);
        if (repository_it == repo_data.end()){
            ErrorUtil::ThrowError("Could not find repository for some schedule");
            return "";
        }
        
        nlohmann::json schedule_json;
        schedule_json["name"] = p.first;
        schedule_json["schedule"] = metaData["schedule"].get<std::string>();
        schedule_json["source"] = metaData["source"].get<std::string>();
        schedule_json["destination"] = repository_it->second->GetRepositoryInfoString();
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
    
    // Silent, no errors thrown if not found
    
    auto it = repo_data.find(schedule_id);
    if (it != repo_data.end()) {
        delete it->second;              
        repo_data.erase(it);            
    }

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