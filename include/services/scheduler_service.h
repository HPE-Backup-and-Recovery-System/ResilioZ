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
        void SendRequest(const char* message);
        void ShowMainMenu();
        void AddSchedule();
        void RemoveSchedule();
        void ViewSchedules();
        void TerminateScheduler();
        
        sockaddr_in serv_addr;
};

#endif