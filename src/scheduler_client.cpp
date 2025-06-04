#include <thread>
#include <chrono>

#include <libcron/Cron.h>
#include <nlohmann/json.hpp>

#include <schedulers/client.h>

int main() {
    Client c;
    c.Run();
    return 0;
}
