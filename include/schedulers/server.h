#ifndef SERVER_H_
#define SERVER_H_

#include <iostream>
#include <atomic>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <map>
#include <vector>

#include <libcron/Cron.h>
#include <nlohmann/json.hpp>

class Server{
    public:
        Server(); // Setup required variables - address, socket creation
        void Run(); // Socket binding to address, listening for connections (loop)

    
    private:
        libcron::Cron <> cron;
        std::map <int, std::string> schedules;
        sockaddr_in address;
        int conn_id = 1;

        std::string addSchedule(nlohmann::json reqBody){
            // Validation pending
            
            // REPLACE with backup logic            
            cron.add_schedule("Schedule id #" + std::to_string(conn_id), reqBody["payload"], [=](auto&) {
                std::cout << "Schduled task id: " << conn_id << " - " << reqBody["type"] << "\n";
            }); 

            nlohmann::json metaData;
            metaData["schedule"] = std::string(reqBody["payload"]);
            metaData["type"] = std::string(reqBody["type"]);

            schedules[conn_id] = metaData.dump();
            std::string message = "Schedule #" + std::to_string(conn_id) + " added!";
            conn_id = conn_id + 1;   
            return message;
        }
        
        std::string viewSchedules(){
            std::string message;
            for (auto p: schedules){
                int id = p.first;
                nlohmann::json metaData = nlohmann::json::parse(p.second);
                
                std::string schedule = metaData["schedule"].get<std::string>();
                std::string type = metaData["type"].get<std::string>();

                message += std::to_string(id) + " - " + schedule + " - " + type + "\n";
            }

            return message;
        }
        
        std::string removeSchedule(nlohmann::json reqBody){
            std::string name = "Schedule id #" + std::to_string(reqBody["payload"].get<int>());
            
            cron.remove_schedule(name);
            schedules.erase(reqBody["payload"].get<int>());

            return name + " removed!";
        }
};

#endif