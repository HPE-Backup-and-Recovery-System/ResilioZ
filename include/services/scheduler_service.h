#ifndef SCHEDULER_SERVICE_H_
#define SCHEDULER_SERVICE_H_

#include <string>

#include <nlohmann/json.hpp>
#include "backup_restore/backup.hpp"
#include "utils/scheduler_request_manager.h"
#include "service.h"

class SchedulerService : public Service {
    public:
        SchedulerService();
        ~SchedulerService();

        void Run() override;
        void Log() override;
        void AttachSchedule(std::string source, std::string destination, BackupType type, std::string remarks);
    
    private:
        void ShowMainMenu();
        void AddSchedule();
        void RemoveSchedule();
        void ViewSchedules();
        
        SchedulerRequestManager *request_mgr;
};

#endif