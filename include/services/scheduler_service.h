#ifndef SCHEDULER_SERVICE_H_
#define SCHEDULER_SERVICE_H_

#include <string>
#include <arpa/inet.h>

#include <nlohmann/json.hpp>
#include "utils/scheduler_request_manager.h"
#include "service.h"

class SchedulerService : public Service {
    public:
        SchedulerService();
        ~SchedulerService();

        void Run() override;
        void Log() override;

    
    private:
        void ShowMainMenu();
        void AddSchedule();
        void RemoveSchedule();
        void ViewSchedules();
        
        sockaddr_in serv_addr;
        SchedulerRequestManager *request_mgr;
};

#endif