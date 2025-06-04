#ifndef SCHEDULER_SERVICE_H_
#define SCHEDULER_SERVICE_H_

#include <string>
#include <arpa/inet.h>

#include <nlohmann/json.hpp>
#include "service.h"

class SchedulerService : public Service {
    public:
        SchedulerService();
        ~SchedulerService() {}

        void Run() override;
        void Log() override;

    
    private:
        void sendRequest(const char* message);
        void showMainMenu();
        void addSchedule();
        void removeSchedule();
        void viewSchedules();
        void terminateScheduler();
        
        sockaddr_in serv_addr;
};

#endif