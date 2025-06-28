#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <libcron/Cron.h>
#include <netinet/in.h>

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include "repositories/all.h"

class Scheduler {
 public:
  Scheduler();
  void Run();

 private:
  libcron::Cron<> cron;
  std::map<std::string, std::string> schedules;
  std::map<std::string, Repository*> repo_data;
  sockaddr_in address;
  int conn_id = 1;

  std::string AddSchedule(nlohmann::json reqBody);
  std::string ViewSchedules();
  std::string RemoveSchedule(nlohmann::json reqBody);

  // Utility functions
  std::string GenerateScheduleName(std::string schedule_id);
  std::string GenerateScheduleId(int conn_id);
  std::string GenerateScheduleInfoString(std::string schedule_id);
};

#endif