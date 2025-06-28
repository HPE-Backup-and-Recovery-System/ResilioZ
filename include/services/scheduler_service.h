#ifndef SCHEDULER_SERVICE_H_
#define SCHEDULER_SERVICE_H_

#include <string>

#include <nlohmann/json.hpp>
#include "backup_restore/backup.hpp"
#include "utils/scheduler_request_manager.h"
#include "repositories/all.h"
#include "services/repository_service.h"
#include "service.h"

class SchedulerService : public Service {
    public:
        SchedulerService();
        ~SchedulerService();

        void Run() override;
        void Log() override;
        void AttachSchedule(std::string source, 
            std::string destination_name,
            std::string destination_path,
            std::string destination_password,
            std::string destination_created_at,
            RepositoryType destination_type,
            BackupType type, std::string remarks);
    
    private:
        void ShowMainMenu();
        void AddSchedule();
        void RemoveSchedule();
        void ViewSchedules();
        
        SchedulerRequestManager *request_mgr;
        RepositoryService *repo_service;
};

#endif