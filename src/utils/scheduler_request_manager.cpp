#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include "backup_restore/backup.hpp"
#include "utils/scheduler_request_manager.h"
#include "schedulers/schedule.h"
#include "utils/error_util.h"
#include "utils/logger.h"

SchedulerRequestManager::SchedulerRequestManager(){
    int port = 8080;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
}

std::string SchedulerRequestManager::SendRequest(const char *message){
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
    }

    send(client_fd, message, strlen(message), 0);

    // Fetch server response and display
    char server_msg[1024];
    if (recv(client_fd, server_msg, sizeof(server_msg), 0 ) < 0){
        ErrorUtil::ThrowError("Message from scheduler server not received!");
    }
    
    close(client_fd);
    std::string response(server_msg);
    return response;
}

std::vector<Schedule> SchedulerRequestManager::SendViewRequest(){
    nlohmann::json reqBody;
    reqBody["action"] = "view";

    std::string response;
    response = SendRequest(reqBody.dump().c_str());
    
    // To do: handle response
    nlohmann::json responseObj = nlohmann::json::parse(response);

    std::vector<Schedule> out;
    for (const auto& item : responseObj["schedules"]) {
        Schedule s;
        s.name = item["name"];
        s.schedule = item["schedule"];
        s.source = item["source"];
        s.destination = item["destination"];
        s.remarks = item["remarks"];
        s.backup_type = item["backup_type"];
        out.push_back(s);
    }
    return out;
}

std::string SchedulerRequestManager::SendAddRequest( std::string schedule,std::string source,
    std::string destination, std::string remarks, BackupType type){

    nlohmann::json reqBody;
    reqBody["action"] = "add";
    reqBody["payload"] = schedule;
    reqBody["source"] = source;
    reqBody["destination"] = destination;
    reqBody["type"] = type;
    reqBody["remarks"] = remarks;
    
    std::string schedule_id;
    schedule_id = SendRequest(reqBody.dump().c_str());
    
    return schedule_id;
}

bool SchedulerRequestManager::SendDeleteRequest(std::string schedule_id){
    nlohmann::json reqBody;
    reqBody["action"] = "remove";
    reqBody["payload"] = schedule_id;
    
    SendRequest(reqBody.dump().c_str());
    std::string response_id;
    response_id = SendRequest(reqBody.dump().c_str());

    if (schedule_id == response_id){
        return true;
    }
    else{
        return false;
    }
}