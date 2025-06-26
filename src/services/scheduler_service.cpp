#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "services/scheduler_service.h"
#include "backup_restore/backup.hpp"
#include "utils/error_util.h"
#include "utils/logger.h"
#include "utils/prompter.h"
#include "utils/user_io.h"

SchedulerService::SchedulerService(){
    request_mgr = new SchedulerRequestManager();
}

SchedulerService::~SchedulerService(){
    delete request_mgr;
}

void SchedulerService::Log(){
    Logger::TerminalLog("Scheduler service is running...");
}

void SchedulerService::AttachSchedule(std::string source, std::string destination, BackupType type, std::string remarks){
    std::string schedule_string = Prompter::PromptCronString();
    std::string schedule_id = request_mgr->SendAddRequest(schedule_string, source, destination, remarks, type);
    
    Logger::Log("Schedule " + schedule_id + " created!");
}

void SchedulerService::AddSchedule(){
    UserIO::DisplayTitle("Adding Schedule");
    std::string schedule, backup_type, source, destination,remarks;
    std::vector<std::string> menu;
    int choice;
    BackupType type;

    schedule = Prompter::PromptCronString();

    source = Prompter::PromptPath("Path to Backup Source");
    destination = Prompter::PromptPath("Path to Backup Destination");
    menu = {"Full Backup", "Incremental Backup", "Differential Backup"};
    choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Backup Type", false), menu);
    type = static_cast<BackupType>(choice);
    remarks = Prompter::PromptInput("Remarks for Backup (Optional)");
    
    std::string schedule_id = request_mgr->SendAddRequest(schedule, source, destination, remarks, type);
    
    Logger::Log("Schedule " + schedule_id + " created!");
}

void SchedulerService::RemoveSchedule(){
    UserIO::DisplayTitle("Removing Schedule");
    std::string schedule_id;
    
    schedule_id = Prompter::PromptScheduleId("Schedule ID");
    
    bool done = request_mgr->SendDeleteRequest(schedule_id);
    if (done){
        Logger::Log("Schedule " + schedule_id + " deleted!");
    }
    else{
        ErrorUtil::ThrowError("Schedule deletion failed!");
    }
}

void SchedulerService::ViewSchedules(){
    UserIO::DisplayTitle("Viewing All Schedules");
    
    std::vector<Schedule> schedules = request_mgr->SendViewRequest();

    if (schedules.empty()){
        Logger::TerminalLog("No schedules yet!");
        return;
    }

    for (auto s: schedules){
        std::string message;
        message += "-- Backup Type: " + s.backup_type + "\n" ;
        message += "-- Backup Schedule: " + s.schedule + "\n";
        message += "-- Source: " + s.source + "\n";
        message += "-- Destination: " + s.destination + "\n";
        message += "-- Remarks: " + s.remarks + "\n\n";

        Logger::TerminalLog(message);
    }
}

void SchedulerService::ShowMainMenu(){
    std::vector<std::string> main_menu = {
      "Go BACK...", "Create New Schedule", "List All Schedules",
      "Delete Schedule"};
    
    while (true) {
        int choice = UserIO::HandleMenuWithSelect(
            UserIO::DisplayMaxTitle("Scheduler Service", false), main_menu);

        try {
        switch (choice) {
            case 0:
            std::cout << "\n - Going Back...\n";
            return;
            case 1:
            AddSchedule();
            break;
            case 2:
            ViewSchedules();
            break;
            case 3:
            RemoveSchedule();
            break;
            default:
            Logger::TerminalLog("Menu Mismatch...", LogLevel::ERROR);
        }
        } catch (const std::exception& e) {
        ErrorUtil::LogException(e, "Scheduler Service");
        }
    }
}

void SchedulerService::Run(){
    // Main user loop
    ShowMainMenu();
}