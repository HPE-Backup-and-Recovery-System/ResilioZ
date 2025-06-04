#include <iostream>
#include <schedulers/scheduler.h>
int main() {
    Scheduler s;
    std::cout << "Starting up scheduler server at port 8080...\n";
    s.Run();
    std::cout << "Scheduler server terminated.\n";
    return 0;
}
