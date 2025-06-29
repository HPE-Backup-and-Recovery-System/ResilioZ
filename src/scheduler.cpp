#include "schedulers/scheduler.h"
#include <csignal>
#include "utils/logger.h"


Scheduler* global_scheduler_ptr = nullptr;

void signal_handler(int signum) {
  Logger::TerminalLog("Initiating shutdown...");
  if (global_scheduler_ptr){
    global_scheduler_ptr->RequestShutdown();
  }
}

int main() {
  Scheduler scheduler;

  global_scheduler_ptr = &scheduler;
  std::signal(SIGTERM, signal_handler);
  std::signal(SIGINT, signal_handler);

  Logger::Log("Starting up scheduler server at Port 55055...");
  scheduler.Run();
  Logger::Log("Scheduler server terminated.");

  return 0;
}
