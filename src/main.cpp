#include <thread>
#include <chrono>

#include <libcron/Cron.h>
#include <nlohmann/json.hpp>

#include <schedulers/server.h>
#include <schedulers/client.h>

int main() {
    // To do: Seperation of server from client logic
    Server s;
    Client c;

    // Currently running server as a seperate thread.
    std::thread scheduler_thread([&]() {
        s.Run();
    });

    // Time delay to make sure request not sent before server available.
    // Should be replaced by health check later
    std::this_thread::sleep_for(std::chrono::seconds(1)); 
    c.Run();

    scheduler_thread.join();
    return 0;
}
