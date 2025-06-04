#include <schedulers/scheduler.h>

#include <iostream>

int main() {
  Scheduler schedule;
  std::cout << "Starting up scheduler server at port 8080...\n";
  schedule.Run();
  std::cout << "Scheduler server terminated.\n";
  return 0;
}
