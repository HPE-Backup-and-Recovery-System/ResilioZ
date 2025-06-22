#ifndef SCHEDULER_REQUEST_MANAGER_H_
#define SCHEDULER_REQUEST_MANAGER_H_

#include <vector>
#include <netinet/in.h>
#include "schedulers/schedule.h"
#include "backup_restore/backup.hpp"

class SchedulerRequestManager{
    public:
        SchedulerRequestManager();

        std::vector<Schedule> SendViewRequest();
        std::string SendAddRequest(
            std::string schedule,
            std::string source,
            std::string destination,
            std::string remarks,
            BackupType type);
        bool SendDeleteRequest(std::string schedule_id);
    
    private:
        std::string SendRequest(const char *message);
        sockaddr_in serv_addr;
        
};


#endif