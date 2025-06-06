#include "schedulers/scheduler.h"

#include <iostream>

#include "utils/logger.h"

int main() {
  Scheduler schedule;

  Logger::Log("Starting up scheduler server at Port 8080...");

  schedule.Run();

  Logger::Log("Scheduler server terminated.");

  return 0;
}
