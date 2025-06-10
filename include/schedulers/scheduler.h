#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <libcron/Cron.h>
#include <netinet/in.h>

#include <map>
#include <nlohmann/json.hpp>
#include <string>

class Scheduler {
 public:
  Scheduler();
  void Run();

 private:
  libcron::Cron<> cron;
  std::map<std::string, std::string> schedules;
  sockaddr_in address;
  int conn_id = 1;

  std::string addSchedule(nlohmann::json reqBody);
  std::string viewSchedules();
  std::string removeSchedule(nlohmann::json reqBody);

  std::string generateScheduleName(std::string schedule_id);
  std::string generateScheduleId(int conn_id);
};

#endif